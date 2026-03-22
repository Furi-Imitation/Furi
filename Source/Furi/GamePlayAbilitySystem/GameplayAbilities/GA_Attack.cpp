#include "GA_Attack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UGA_Attack::UGA_Attack()
{
    // 어빌리티 기본 정책 설정
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    // 입력 라우팅을 위한 스킬 태그 등록
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Action.Attack")));
}

void UGA_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // 콤보 상태 초기화
    CurrentComboIndex = 1;
    bComboWindowOpened = false;
    bNextComboReserved = false;

    // 1. 콤보 윈도우 시작 이벤트 대기 (ANS_ComboWindow에서 보냄)
    UAbilityTask_WaitGameplayEvent* WaitStartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(FName("Combo.Start")));
    WaitStartTask->EventReceived.AddDynamic(this, &UGA_Attack::OnComboEventReceived);
    WaitStartTask->ReadyForActivation();

    // 2. 콤보 윈도우 종료 이벤트 대기 (ANS_ComboWindow에서 보냄)
    UAbilityTask_WaitGameplayEvent* WaitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(FName("Combo.End")));
    WaitEndTask->EventReceived.AddDynamic(this, &UGA_Attack::OnComboEventReceived);
    WaitEndTask->ReadyForActivation();
    
    // 3. 실제 타격 판정 이벤트 대기 (AN_SendGameplayEvent에서 보냄)
    UAbilityTask_WaitGameplayEvent* HitCheckTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitCheckEventTag);
    HitCheckTask->EventReceived.AddDynamic(this, &UGA_Attack::OnComboEventReceived);
    HitCheckTask->ReadyForActivation();
    
    if (AActor* MyAvatar = GetAvatarActorFromActorInfo())
    {
        RotateTowardsClosestEnemy(MyAvatar, AttackRange);
    }
    
    // 첫 공격 몽타주 실행
    PlayComboSection();
}

void UGA_Attack::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
    Super::InputPressed(Handle, ActorInfo, ActivationInfo);

    // 콤보 입력 가능 구간이고, 아직 예약 안 했고, 3타 이하일 때
    if (bComboWindowOpened && !bNextComboReserved && CurrentComboIndex < 3)
    {
        bNextComboReserved = true; // 예약 완료 깃발 올림

        if (AActor* MyAvatar = GetAvatarActorFromActorInfo())
        {
            RotateTowardsClosestEnemy(MyAvatar, AttackRange);
        }
        
        // 현재 재생 중인 섹션 이름과 다음 이어질 섹션 이름 생성
        FName CurrentSection = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex);
        FName NextSection = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex + 1);
        
        // 현재 모션이 끝나면 멈추지 말고 자연스럽게 다음 타수로 이어주라고 예약!
        MontageSetNextSectionName(CurrentSection, NextSection);
        
        // 다음 타수 준비
        CurrentComboIndex++; 
        
        UE_LOG(LogTemp, Warning, TEXT("Smooth Combo Reserved! %s -> %s"), *CurrentSection.ToString(), *NextSection.ToString());
    }
}

void UGA_Attack::OnComboEventReceived(FGameplayEventData Payload)
{
    FGameplayTag EventTag = Payload.EventTag;

    // 1. 콤보 입력 가능 구간 시작
    if (EventTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Combo.Start"))))
    {
        bComboWindowOpened = true;
        bNextComboReserved = false; // 새로운 구간이 열렸으니 예약 상태 초기화
    }
    // 2. 콤보 입력 가능 구간 끝
    else if (EventTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Combo.End"))))
    {
        bComboWindowOpened = false;
        // (점프 로직은 MontageSetNextSectionName이 대신해주므로 삭제됨)
    }
    // 3. 실제 타격 판정 순간!
    else if (EventTag.MatchesTag(HitCheckEventTag))
    {
        PerformHitCheck();
    }
}

void UGA_Attack::PlayComboSection()
{
    if (!ComboMontage) return;

    // 무조건 Attack1부터 시작합니다.
    FName SectionName = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex);
    
    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("AttackTask"), ComboMontage, 1.0f, SectionName);
    MontageTask->OnCompleted.AddDynamic(this, &UGA_Attack::OnMontageCompleted);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_Attack::OnMontageCompleted);
    MontageTask->ReadyForActivation();
}

void UGA_Attack::PerformHitCheck()
{
    // 1. 기초 데이터 및 내 Ability System Component(ASC) 준비
    AActor* MyAvatar = GetAvatarActorFromActorInfo();
    UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
    
    // 캐릭터 메쉬 소켓에 VFX를 붙이기 위해 캐스팅
    ACharacter* MyCharacter = Cast<ACharacter>(MyAvatar);

    if (!MyAvatar || !MyASC || !MyCharacter) return;

    // --- 2. 물리 판정 (가상 상자 계산) ---
    FVector AvatarLocation = MyAvatar->GetActorLocation();   
    FRotator AvatarRotation = MyAvatar->GetActorRotation(); 
    FVector Forward = MyAvatar->GetActorForwardVector();     

    // 상자 크기 및 중심 계산
    FVector BoxHalfExtent = FVector(AttackRange / 2.0f, AttackBoxHalfWidth, AttackBoxHalfHeight); 
    FVector BoxCenter = AvatarLocation + (Forward * (AttackRange / 2.0f));

    // 캐릭터 발밑 높이 조정 (발바닥부터 판정)
    if (UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(MyAvatar->GetRootComponent()))
    {
        BoxCenter.Z = AvatarLocation.Z - Capsule->GetScaledCapsuleHalfHeight() + BoxHalfExtent.Z; 
    }

    // Overlap 물리 판정 실행
    TArray<FOverlapResult> OverlapResults;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(MyAvatar); // 나는 제외

    FCollisionShape BoxShape = FCollisionShape::MakeBox(BoxHalfExtent);

    //ECC_Pawn 채널을 사용해 주변 캐릭터 탐색
    bool bOverlapHit = GetWorld()->OverlapMultiByChannel(
        OverlapResults,
        BoxCenter,
        AvatarRotation.Quaternion(), 
        ECC_Pawn, 
        BoxShape,
        Params
    );

    // 가장 가까운 적(Pawn) 하나만 찾기
    AActor* ClosestTarget = nullptr;
    float ClosestDistanceSq = FMath::Square(AttackRange + 100.0f); // 초기값은 최대 사거리 바깥

    if (bOverlapHit)
    {
        for (const FOverlapResult& Result : OverlapResults)
        {
            AActor* OverlappedActor = Result.GetActor();
            // 배경/바닥 제외하고 실제 캐릭터(Pawn)인지 확인
            if (OverlappedActor && OverlappedActor->IsA<APawn>())
            {
                float DistanceSq = FVector::DistSquared(AvatarLocation, OverlappedActor->GetActorLocation());
                if (DistanceSq < ClosestDistanceSq)
                {
                    ClosestDistanceSq = DistanceSq;
                    ClosestTarget = OverlappedActor; 
                }
            }
        }
    }

    // --- 3. 결과 처리 ---
    if (ClosestTarget)
    {
        // =========================================================================
        // 🌟 [사용자님 요청: 분리된 연출 실행] ExecuteGameplayCue 방식으로 즉시 실행
        // =========================================================================

        // 공통 파라미터 설정
        FGameplayCueParameters GcParams;
        GcParams.Instigator = MyAvatar; // 공격자 (나)
        GcParams.EffectCauser = MyAvatar;
        GcParams.Location = ClosestTarget->GetActorLocation(); // 터질 위치 (적 위치)
        
        // 🌟 [VFX 자동 소켓 부착 핵심 설정]
        // 블루프린트에 적힌 "LeftHandSocket" 등을 내 캐릭터 메쉬에서 찾으라는 뜻입니다.
        // 트레일 이펙트라면 내 메쉬를 넘겨줍니다.
        GcParams.TargetAttachComponent = MyCharacter->GetMesh(); 

        // 콤보 타수(Small/Large)에 따라 VFX/SFX 태그 결정
        FGameplayTag VFXTag, SFXTag;
        if (CurrentComboIndex == 3) // 3타 피니시 (Large)
        {
            VFXTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.VFX.Attack.Large"));
            SFXTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.SFX.Attack.Large"));
        }
        else // 1~2타 (Small)
        {
            VFXTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.VFX.Attack.Small"));
            SFXTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.SFX.Attack.Small"));
        }
        
        // 🌟 [중요: 카메라 쉐이크] 모든 적중 시 카메라 쉐이크 전용 큐 실행
        FGameplayTag ShakeTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.CameraShake.Hit"));

        // 내 ASC가 직접 각각의 분리된 큐를 호출합니다.
        // (블루프린트 파일들이 각 태그와 매핑되어 있어야 작동합니다.)
        
        // 1. VFX 실행 -> 양쪽 소켓 트레일 터짐
        MyASC->ExecuteGameplayCue(VFXTag, GcParams); 
        
        // 2. SFX 실행 (사운드 전용 큐 실행) -> 타격음 터짐
        MyASC->ExecuteGameplayCue(SFXTag, GcParams); 
        
        // 3. Shake 실행 (쉐이크 전용 큐 실행) -> 화면 흔들림 터짐
        MyASC->ExecuteGameplayCue(ShakeTag, GcParams); 

        // --- 4. 대미지 적용 (GameplayEffect 사용) ---
        UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ClosestTarget);
        if (TargetASC)
        {
            // 콤보 맵에서 GE 클래스 가져오기
            TSubclassOf<UGameplayEffect> DamageGE = ComboDamageMap.Contains(CurrentComboIndex) ? ComboDamageMap[CurrentComboIndex] : nullptr;
            
            if (DamageGE)
            {
                FGameplayEffectContextHandle Context = MyASC->MakeEffectContext();
                FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(DamageGE, 1.0f, Context);

                if (SpecHandle.IsValid())
                {
                    // 대미지 수치 설정 (Caller Magnitude)
                    float BaseDamage = (CurrentComboIndex == 3) ? -30.f : -10.f; 
                    SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")), BaseDamage);

                    // 피격 응답 설정 (HitReaction / KnockBack)
                    float ResponseValue = (CurrentComboIndex == 3) ? (float)EFuriDamageResponse::KnockBack : (float)EFuriDamageResponse::HitReaction;
                    SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage.Response")), ResponseValue);

                    // 적에게 순수하게 대미지만 적용 (연출은 위에서 끝냈음)
                    TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
                    
                    UE_LOG(LogTemp, Log, TEXT("Hit Success! Combo %d Applied to: %s"), CurrentComboIndex, *ClosestTarget->GetName());
                }
            }
        }
    }
    else
    {
        // 🌟 [허공 스윙] 적중 실패 시 사운드만 재생 (큐가 아닌 일반 사운드로 처리)
        PlaySwingSound();
        UE_LOG(LogTemp, Log, TEXT("Hit Missed - Playing Swing Sound"));
    }

    // 디버그 드로잉 (에디터 전용)
#if !UE_BUILD_SHIPPING 
    FColor DebugColor = (ClosestTarget != nullptr) ? FColor::Red : FColor::Green;
    DrawDebugBox(GetWorld(), BoxCenter, BoxHalfExtent, AvatarRotation.Quaternion(), DebugColor, false, 1.0f, 0, 2.0f);
#endif
}

void UGA_Attack::OnMontageCompleted()
{
    // 예약된 연타가 없어서 몽타주가 완전히 끝났다면 어빌리티도 깔끔하게 종료
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Attack::RotateTowardsClosestEnemy(AActor* MyAvatar, float SearchRadius)
{
    if (!MyAvatar || !GetWorld()) return;

    FVector AvatarLocation = MyAvatar->GetActorLocation();

    // --- 1. 내 주변 구형 범위 내의 모든 Pawn 탐색 (Overlap) ---
    TArray<FOverlapResult> OverlapResults;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(MyAvatar); // 나는 제외

    // 검색 범위는 기존 공격 사거리(AttackRange)를 그대로 활용합니다.
    FCollisionShape SphereShape = FCollisionShape::MakeSphere(SearchRadius + 3000);

    bool bOverlapHit = GetWorld()->OverlapMultiByChannel(
        OverlapResults,
        AvatarLocation,
        FQuat::Identity, // 구형이라 회전 안 함
        ECC_Pawn, // 캐릭터 채널
        SphereShape,
        Params
    );

    // --- 2. 가장 가까운 적 찾기 로직 ---
    AActor* ClosestEnemy = nullptr;
    float ClosestDistanceSq = FMath::Square(SearchRadius + 3000.0f); // 아주 큰 거리로 초기화

    if (bOverlapHit)
    {
        for (const FOverlapResult& Result : OverlapResults)
        {
            AActor* OverlappedActor = Result.GetActor();
    
            if (OverlappedActor && OverlappedActor->IsA<APawn>())
            {
                float DistanceSq = FVector::DistSquared(AvatarLocation, OverlappedActor->GetActorLocation());
                if (DistanceSq < ClosestDistanceSq)
                {
                    ClosestDistanceSq = DistanceSq;
                    ClosestEnemy = OverlappedActor;
                }
            }
        }
    }

    // --- 3. 찾았다면 그 적을 바라보도록 회전 시키기 ---
    if (ClosestEnemy)
    {
        // 내 위치에서 적의 위치를 바라보는 회전값(FRotator) 계산
        FVector TargetLocation = ClosestEnemy->GetActorLocation();
        
        // Z축(높이)은 무시하고 평면상에서만 회전하도록 만듭니다. (안 그러면 적 발바닥이나 가슴팍을 향해 캐릭터가 비스듬히 눕습니다.)
        TargetLocation.Z = AvatarLocation.Z; 
        
        // [함수: FindLookAtRotation] 시작점(내 위치)에서 끝점(적 위치)을 바라보는 완벽한 회전값을 구해줌
        FRotator LookAtRotation = FRotationMatrix::MakeFromX(TargetLocation - AvatarLocation).Rotator();

        // 캐릭터의 월드 회전값을 즉시 변경합니다!
        // 콤보 1타 애니메이션이 시작되기 직전이므로, 1타 모션이 홱 돌아간 상태에서 정확히 적을 향해 시작됩니다.
        MyAvatar->SetActorRotation(LookAtRotation);

#if !UE_BUILD_SHIPPING
        // 디버그용: 자동 회전 시 1초 동안 파란색 선 표시
        DrawDebugLine(GetWorld(), AvatarLocation, ClosestEnemy->GetActorLocation(), FColor::Blue, false, 1.0f, 0, 3.0f);
        UE_LOG(LogTemp, Log, TEXT("Auto-Targeting Rotation Applied to: %s"), *ClosestEnemy->GetName());
#endif
    }
}

// 허공 사운드 재생 함수 구현
void UGA_Attack::PlaySwingSound()
{
    if (SwingSound)
    {
        // 아바타의 현재 위치에서 사운드 재생
        UGameplayStatics::PlaySoundAtLocation(this, SwingSound, GetAvatarActorFromActorInfo()->GetActorLocation());
    }
}
