#include "SSRFinal.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Furi/SSR/SSRPlayer.h" 
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"

USSRFinal::USSRFinal()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void USSRFinal::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    bHitConfirmed = false;
    GrabbedTarget = nullptr;

    CurrentHitCount = 0;
    
    // 1. 몽타주 실행
    UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, TEXT("SSRFinalTask"), FinalAttackMontage, 1.0f, FName("Trigger"));
    
    // 몽타주 종료 시 어빌리티도 종료되도록 연결
    MontageTask->OnCompleted.AddDynamic(this, &USSRFinal::OnMontageFinished);
    MontageTask->OnInterrupted.AddDynamic(this, &USSRFinal::OnMontageFinished);
    MontageTask->ReadyForActivation();

    // 2. 판정 시작 알림 대기
    UAbilityTask_WaitGameplayEvent* CollisionTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(FName("Event.Hit.Check")));
    CollisionTask->EventReceived.AddDynamic(this, &USSRFinal::OnActivateCollision);
    CollisionTask->ReadyForActivation();

    // 3. 적중 성공 알림 대기
    UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(FName("Event.SSRFinal.HitSuccess")));
    HitTask->EventReceived.AddDynamic(this, &USSRFinal::OnHitSuccess);
    HitTask->ReadyForActivation();

    // 4. 적중 실패 알림 대기
    UAbilityTask_WaitGameplayEvent* FailedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(FName("Event.SSRFinal.GoToFailed")));
    FailedTask->EventReceived.AddDynamic(this, &USSRFinal::OnAttackFailed);
    FailedTask->ReadyForActivation();
    
    // 5. [추가] 데미지 노티파이 대기 (몽타주에서 쏜 데미지 태그)
    UAbilityTask_WaitGameplayEvent* DamageTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(FName("Event.SSRFinal.DoDamage")));
    DamageTask->EventReceived.AddDynamic(this, &USSRFinal::OnDamageNotifyReceived);
    DamageTask->ReadyForActivation();
    
}

void USSRFinal::OnActivateCollision(FGameplayEventData Payload)
{
    // 1. 현재 어빌리티 상태 확인
    if (!IsActive()) return;

    // 2. AvatarActor를 직접 가져와서 유효성 검사
    AActor* Avatar = (CurrentActorInfo != nullptr) ? CurrentActorInfo->AvatarActor.Get() : nullptr;
    if (!Avatar) return;

    // 3. 캐스팅 후 함수 호출 전 최종 확인
    ASSRPlayer* SSRPlayer = Cast<ASSRPlayer>(Avatar);
    if (SSRPlayer)
    {
        // 여기서 터진다면 SSRPlayer 내부를 확인해야 함
        SSRPlayer->SetUltimateCollisionEnabled(true);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SSRFinal: Avatar is not ASSRPlayer!"));
    }
}

void USSRFinal::OnHitSuccess(FGameplayEventData Payload)
{
    if (bHitConfirmed) return;

    ASSRPlayer* SSRPlayer = Cast<ASSRPlayer>(GetAvatarActorFromActorInfo());
    AActor* TargetActor = const_cast<AActor*>(Cast<AActor>(Payload.Target));

    // 로그 1: 적중 시도 시 타겟 유효성 확인
    UE_LOG(LogTemp, Warning, TEXT("SSRFinal: OnHitSuccess Called. Target: %s"), TargetActor ? *TargetActor->GetName() : TEXT("NULL"));

    if (SSRPlayer && TargetActor && IsValid(TargetActor))
    {
        bHitConfirmed = true;
        GrabbedTarget = TargetActor;

        // GrabbedTarget->AttachToComponent(SSRPlayer->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        
        // SSRPlayer->GetMesh() 대신 GetRootComponent()에 붙이면 애니메이션의 영향을 덜 받습니다.
        GrabbedTarget->AttachToComponent(SSRPlayer->GetRootComponent(), 
            FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        
        if (ACharacter* TargetChar = Cast<ACharacter>(GrabbedTarget))
        {
            TargetChar->GetCharacterMovement()->DisableMovement();
            TargetChar->GetCharacterMovement()->StopMovementImmediately();
        }
        
        // 좌표 설정 (로그 3: 상대 좌표 적용 확인)
        FVector RelativeOffset = FVector(0.f, 60.f, 50.f); 
        GrabbedTarget->SetActorRelativeLocation(RelativeOffset);
        
        // GrabbedTarget->SetActorRelativeLocation(FVector(100.f,0.f,50.f));
        
        FVector AfterAttachPos = GrabbedTarget->GetActorLocation();
        UE_LOG(LogTemp, Log, TEXT("SSRFinal: After Attach (World) Location: %s"), *AfterAttachPos.ToString());

        SSRPlayer->SetCameraZoom(true);
        MontageJumpToSection(FName("Combo"));
    }
}

void USSRFinal::ApplyDamageToGrabbedTarget(float InDamageAmount)
{
    // 로그 4: 타겟 유효성 및 포인터 주소 확인
    if (!GrabbedTarget)
    {
        UE_LOG(LogTemp, Error, TEXT("SSRFinal: ApplyDamage Failed! GrabbedTarget is NULL."));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("SSRFinal: Applying Damage to %s. Current Hit: %d"), *GrabbedTarget->GetName(), CurrentHitCount);

    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GrabbedTarget);
    UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();

    if (MyASC && TargetASC)
    {
        FGameplayEffectContextHandle ContextHandle = MyASC->MakeEffectContext();
        ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());

        // 📍 로그 5: 실제 이펙트가 터져야 할 월드 좌표 추출
        FVector TargetLoc = GrabbedTarget->GetActorLocation();
        ContextHandle.AddOrigin(TargetLoc);
        
        // 시각적 디버깅: 게임 화면에 빨간 구체를 1초간 그려서 위치 표시
        DrawDebugSphere(GetWorld(), TargetLoc, 20.f, 12, FColor::Red, false, 1.0f);
        UE_LOG(LogTemp, Warning, TEXT("SSRFinal: GameplayCue/Damage Origin Location: %s"), *TargetLoc.ToString());

        FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), ContextHandle);
        
        if (SpecHandle.IsValid())
        {
            FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount"));
            UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, DamageTag, InDamageAmount);

            MyASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
             
            FGameplayTag CueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.SSR.Final.Combo"));
            FGameplayCueParameters CueParams;
            CueParams.Instigator = GetAvatarActorFromActorInfo();
            CueParams.Location = TargetLoc; // 보정된 위치 사용

            MyASC->ExecuteGameplayCue(CueTag, CueParams);
        }
    }
}

void USSRFinal::OnDamageNotifyReceived(FGameplayEventData Payload)
{
    CurrentHitCount++;

    float DamageToApply = -10.0f; // 기본 데미지

    // 예: 5번째 타격(막타)은 3배 데미지!
    if (CurrentHitCount >= 5)
    {
        DamageToApply = -30.0f;
    }

    ApplyDamageToGrabbedTarget(DamageToApply);
}

void USSRFinal::OnAttackFailed(FGameplayEventData Payload)
{
    if (!bHitConfirmed)
    {
        MontageJumpToSection(FName("Failed"));
        
        if (ASSRPlayer* SSRPlayer = Cast<ASSRPlayer>(GetAvatarActorFromActorInfo()))
        {
            SSRPlayer->SetUltimateCollisionEnabled(false);
        }
    }
}

void USSRFinal::OnMontageFinished()
{
    // 몽타주가 완료되면 어빌리티 종료
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USSRFinal::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    // 🌟 마무리 정리 로직
    ASSRPlayer* SSRPlayer = Cast<ASSRPlayer>(GetAvatarActorFromActorInfo());
    if (SSRPlayer)
    {
        SSRPlayer->SetUltimateCollisionEnabled(false);
        SSRPlayer->SetCameraZoom(false); // 카메라 복구
    }

    if (GrabbedTarget)
    {
        // 고정했던 타겟 풀어주기
        GrabbedTarget->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        
        if (ACharacter* TargetChar = Cast<ACharacter>(GrabbedTarget))
        {
            TargetChar->GetCharacterMovement()->SetDefaultMovementMode();
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
