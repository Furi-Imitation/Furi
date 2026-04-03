#include "SSRFinal.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Furi/SSR/SSRPlayer.h" 
#include "Furi/FuriPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h" // BoxTrace를 사용하기 위해 필수

USSRFinal::USSRFinal()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void USSRFinal::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // 1. 초기화 및 캐싱
    CachedPlayer = Cast<ASSRPlayer>(ActorInfo->AvatarActor.Get());
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || !CachedPlayer)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    bFirstHitSuccess = false;
    GrabbedTarget = nullptr;
    CurrentHitCount = 0;

    // 3. 통합 이벤트 대기 (몽타주 노티파이 신호 수신)
    if (HitEventTag.IsValid())
    {
        UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitEventTag);
        WaitEventTask->EventReceived.AddDynamic(this, &USSRFinal::OnHitEventReceived);
        WaitEventTask->ReadyForActivation();
        
        UE_LOG(LogTemp, Log, TEXT("SSRFinal: Waiting for Tag [%s]"), *HitEventTag.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SSRFinal: HitEventTag is NOT valid in Blueprint!"));
    }
    
    // 2. 몽타주 실행 태스크
    UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, TEXT("UltimateTask"), UltimateMontage);
    MontageTask->OnCompleted.AddDynamic(this, &USSRFinal::OnMontageFinished);
    MontageTask->OnInterrupted.AddDynamic(this, &USSRFinal::OnMontageFinished);
    MontageTask->ReadyForActivation();

}

void USSRFinal::OnHitEventReceived(FGameplayEventData Payload)
{
    // 노티파이가 올 때마다 물리 판정 및 데미지 로직 실행
    ProcessPhysicalHit();
}

void USSRFinal::ProcessPhysicalHit()
{
    if (!IsActive() || !CachedPlayer) return;

    // [A] 이미 적을 잡은 상태인지 확인 (콤보 구간)
    if (bFirstHitSuccess)
    {
        if (IsValid(GrabbedTarget))
        {
            CurrentHitCount++;
            float DamageToApply = (CurrentHitCount >= 5) ? -30.0f : -10.0f;
            ApplyDamageToTarget(DamageToApply);
            if (HitComboEffect)
            {
                // 타겟의 위치에 이펙트 소환
                UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitComboEffect, GrabbedTarget->GetActorLocation());
            }
            return; 
        }
        return;
    }

    // [B] 적 탐색 (첫 타격 판정)
    FVector Start = CachedPlayer->GetActorLocation();
    FVector Forward = CachedPlayer->GetActorForwardVector();
    FVector End = Start + (Forward * AttackRange);
    FVector HalfSize = FVector(AttackRange * 0.5f, AttackBoxHalfWidth, AttackBoxHalfHeight);

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(CachedPlayer);

    FHitResult HitResult;
    bool bHit = UKismetSystemLibrary::BoxTraceSingle(
        GetWorld(), Start, End, HalfSize, CachedPlayer->GetActorRotation(),
        UEngineTypes::ConvertToTraceType(ECC_Pawn), false, 
        ActorsToIgnore, EDrawDebugTrace::ForDuration, HitResult, true, 
        FLinearColor::Red, FLinearColor::Green, 1.0f
    );

    // 🌟 수정된 구간: 변수 중복 선언 해결
    if (bHit && HitResult.GetActor())
    {
        // 1. 여기서 먼저 캐릭터인지 체크
        ACharacter* TargetChar = Cast<ACharacter>(HitResult.GetActor());
        
        if (TargetChar) 
        {
            bFirstHitSuccess = true;
            GrabbedTarget = TargetChar;
            
            // 필살기 적중시 카메라 확대 구도
            if (AFuriPlayerController* pc = Cast<AFuriPlayerController>(CurrentActorInfo->PlayerController.Get()))
            {
                pc->SetCinematicMode(true, TargetChar);
            }
            
            // 슬로우 모션
            UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.3f);
            
            // 2. 박제 로직 (위에서 만든 TargetChar 변수를 그대로 사용)
            GrabbedTarget->AttachToComponent(CachedPlayer->GetRootComponent(), 
                FAttachmentTransformRules::SnapToTargetNotIncludingScale);
            
            GrabbedTarget->SetActorRelativeLocation(FVector(120.f, 0.f, 0.f));
            GrabbedTarget->SetActorRelativeRotation(FRotator(0.f, 180.f, 0.f));

            
            // 캐릭터 이동 컴포넌트 제어 (중복 선언 제거됨)
            TargetChar->GetCharacterMovement()->DisableMovement();
            TargetChar->GetCharacterMovement()->StopMovementImmediately();

            CachedPlayer->SetCameraZoom(true);
            
            if (GrabEffect)
            {
                // 타겟에게 이펙트를 부착 (적을 따라다님)
                UNiagaraFunctionLibrary::SpawnSystemAttached(GrabEffect, GrabbedTarget->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
            }
            
            UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
            if (MyASC) MyASC->CurrentMontageJumpToSection(FName("Combo"));
        }
    }
    else
    {
        // 빗나갔을 때
        UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
        if (MyASC) MyASC->CurrentMontageJumpToSection(FName("Failed"));
    }
}

void USSRFinal::ApplyDamageToTarget(float Amount)
{
    UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GrabbedTarget);

    if (MyASC && TargetASC && DamageEffectClass)
    {
        FGameplayEffectContextHandle Context = MyASC->MakeEffectContext();
        FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), Context);
        
        if (SpecHandle.IsValid())
        {
            FGameplayTag DamageDataTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount"));
            UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, DamageDataTag, Amount);
            MyASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
        }
    }
}

void USSRFinal::OnMontageFinished()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USSRFinal::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (CachedPlayer)
    {
        CachedPlayer->SetCameraZoom(false);
    }

    if (GrabbedTarget)
    {
        GrabbedTarget->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        if (ACharacter* TargetChar = Cast<ACharacter>(GrabbedTarget))
        {
            TargetChar->GetCharacterMovement()->SetDefaultMovementMode();
        }
    }
    
    // 필살기 종류 후 카메라 원상복귀
    if (GetWorld())
    {
        UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
    }
    
    if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(CurrentActorInfo->PlayerController.Get()))
    {
        PC->SetCinematicMode(false, nullptr);
    }
    

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}