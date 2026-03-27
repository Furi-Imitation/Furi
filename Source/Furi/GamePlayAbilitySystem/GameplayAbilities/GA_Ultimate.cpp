#include "GA_Ultimate.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Furi/FuriPlayerController.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/utils/FuriTypes.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"

UGA_Ultimate::UGA_Ultimate()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Lock")));
    
    HitEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Hit.Check"));
    UltimateFlashTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.VFX.UltimateFlash"));
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Lock")));

}

void UGA_Ultimate::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    bFirstHitSuccess = false;
    CurrentHitCount = 0;

    // 🚀 [Launch 제거] 이제 캐릭터는 제자리 혹은 애니메이션 루트 모션에 따라 움직입니다.

    // 1. 타격 이벤트 대기
    UAbilityTask_WaitGameplayEvent* WaitHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitEventTag);
    WaitHitTask->EventReceived.AddDynamic(this, &UGA_Ultimate::OnHitEventReceived);
    WaitHitTask->ReadyForActivation();

    // 2. 몽타주 실행
    UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, TEXT("UltTask"), UltimateMontage, 1.0f, TEXT("Dash"));
    PlayTask->OnCompleted.AddDynamic(this, &UGA_Ultimate::OnMontageFinished);
    PlayTask->OnInterrupted.AddDynamic(this, &UGA_Ultimate::OnMontageFinished);
    PlayTask->OnCancelled.AddDynamic(this, &UGA_Ultimate::OnMontageFinished);
    PlayTask->ReadyForActivation();
}

void UGA_Ultimate::OnHitEventReceived(FGameplayEventData Payload)
{
    // 노티파이가 올 때마다 물리 판정 실행
    ProcessPhysicalHit();
}

void UGA_Ultimate::OnMontageFinished()
{
    UE_LOG(LogTemp, Log, TEXT("Ultimate Montage Finished. Ending Ability..."));
    
    // EndAbility를 호출해야 캐릭터의 State.Lock 태그가 제거되고 다시 움직일 수 있습니다.
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Ultimate::ProcessPhysicalHit()
{
    AActor* MyAvatar = GetAvatarActorFromActorInfo();
    UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
    if (!MyAvatar || !MyASC) return;

    // --- 물리 판정 로직 (기존 박스 트레이스 동일) ---
    FVector AvatarLocation = MyAvatar->GetActorLocation();   
    FRotator AvatarRotation = MyAvatar->GetActorRotation(); 
    FVector Forward = MyAvatar->GetActorForwardVector();     
    FVector BoxHalfExtent = FVector(AttackRange / 2.0f, AttackBoxHalfWidth, AttackBoxHalfHeight); 
    FVector BoxCenter = AvatarLocation + (Forward * (AttackRange / 2.0f));

    if (UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(MyAvatar->GetRootComponent()))
    {
        BoxCenter.Z = AvatarLocation.Z - Capsule->GetScaledCapsuleHalfHeight() + BoxHalfExtent.Z; 
    }

    TArray<FOverlapResult> OverlapResults;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(MyAvatar); 
    FCollisionShape BoxShape = FCollisionShape::MakeBox(BoxHalfExtent);
    bool bOverlapHit = GetWorld()->OverlapMultiByChannel(OverlapResults, BoxCenter, AvatarRotation.Quaternion(), ECC_Pawn, BoxShape, QueryParams);

    AActor* ClosestTarget = nullptr;
    float ClosestDistanceSq = FMath::Square(AttackRange + 100.0f);

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
                    ClosestTarget = OverlappedActor; 
                }
            }
        }
    }

    // --- 🌟 타격 성공 시 처리 ---
    if (ClosestTarget)
    {
        CurrentHitCount++; // 타격 성공 시 카운트 증가

        // [1타 적중 시] 시네마틱 모드 전환 및 난무 섹션 점프
        if (CurrentHitCount == 1 && !bFirstHitSuccess)
        {
            bFirstHitSuccess = true;
            if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(CurrentActorInfo->PlayerController.Get()))
            {
                PC->SetCinematicMode(true, ClosestTarget);
            }
            // 🌟 에디터에서 Dash -> Whiff로 연결해두었으므로, 여기서 강제로 Combo 섹션으로 보내줍니다.
            MontageJumpToSection(TEXT("Cinematic_Combo"));
        }

        // --- Gameplay Cue 및 사운드 ---
        FGameplayCueParameters HitParams;
        HitParams.Instigator = MyAvatar;
        HitParams.Location = ClosestTarget->GetActorLocation();

        MyASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.VFX.Hit")), HitParams);

        // 🌟 [막타 판정] Dash(1) + Combo(3) = 총 4타가 막타
        bool bIsFinisher = (CurrentHitCount == 4);
        
        FGameplayTag HitSFXTag = bIsFinisher ? 
            FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.SFX.Attack.Large")) : 
            FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.SFX.Attack.Small"));
        MyASC->ExecuteGameplayCue(HitSFXTag, HitParams);

        // [막타 전용 연출: Flash + CameraShake]
        if (bIsFinisher)
        {
            MyASC->ExecuteGameplayCue(UltimateFlashTag, HitParams);
            UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.05f);
            FTimerHandle TimerHandle;
            GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([this]() {
                UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
            }), 0.15f, false);
        }

        // --- 대미지 적용 ---
        AGasCharacterBase* TargetCharacter = Cast<AGasCharacterBase>(ClosestTarget);
        if (TargetCharacter && DamageEffectClass)
        {
            FFuriDamageInfo DamageInfo;
            DamageInfo.Amount = bIsFinisher ? -70.f : -15.f; // 4타 대미지 강화
            DamageInfo.DamageType = EFuriDamageType::Melee;
            DamageInfo.DamageResponse = bIsFinisher ? EFuriDamageResponse::KnockBack : EFuriDamageResponse::HitReaction;
            
            DamageInfo.bCanBeParried = (CurrentHitCount < 4); 
            DamageInfo.bCanBeBlocked = true;
            DamageInfo.bShouldForceInterrupt = bIsFinisher;

            FGameplayEffectContextHandle Context = MyASC->MakeEffectContext();
            FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, Context);
            
            if (SpecHandle.IsValid())
            {
                SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")), DamageInfo.Amount);
                TargetCharacter->TakeFuriDamage(DamageInfo, SpecHandle, MyAvatar);
            }
        }
    }
}

void UGA_Ultimate::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(CurrentActorInfo->PlayerController.Get()))
    {
        PC->SetCinematicMode(false);
    }
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
