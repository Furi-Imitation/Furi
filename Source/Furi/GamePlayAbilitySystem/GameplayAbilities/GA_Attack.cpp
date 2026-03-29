#include "GA_Attack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/GamePlayAbilitySystem/FuriAbilityTypes.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UGA_Attack::UGA_Attack()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Lock")));
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Lock")));
}

void UGA_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    CurrentComboIndex = 1;
    bComboWindowOpened = false;
    bNextComboReserved = false;

    UAbilityTask_WaitGameplayEvent* WaitStartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(FName("Combo.Start")));
    WaitStartTask->EventReceived.AddDynamic(this, &UGA_Attack::OnComboEventReceived);
    WaitStartTask->ReadyForActivation();

    UAbilityTask_WaitGameplayEvent* WaitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(FName("Combo.End")));
    WaitEndTask->EventReceived.AddDynamic(this, &UGA_Attack::OnComboEventReceived);
    WaitEndTask->ReadyForActivation();
    
    UAbilityTask_WaitGameplayEvent* HitCheckTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitCheckEventTag);
    HitCheckTask->EventReceived.AddDynamic(this, &UGA_Attack::OnComboEventReceived);
    HitCheckTask->ReadyForActivation();
    
    if (AActor* MyAvatar = GetAvatarActorFromActorInfo())
    {
        RotateTowardsClosestEnemy(MyAvatar, AttackRange);
    }
    
    PlayComboSection();
}

void UGA_Attack::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
    Super::InputPressed(Handle, ActorInfo, ActivationInfo);

    if (bComboWindowOpened && !bNextComboReserved && CurrentComboIndex < 3)
    {
        bNextComboReserved = true; 

        if (AActor* MyAvatar = GetAvatarActorFromActorInfo())
        {
            RotateTowardsClosestEnemy(MyAvatar, AttackRange);
        }
        
        FName CurrentSection = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex);
        FName NextSection = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex + 1);
        
        MontageSetNextSectionName(CurrentSection, NextSection);
        CurrentComboIndex++; 
    }
}

void UGA_Attack::OnComboEventReceived(FGameplayEventData Payload)
{
    FGameplayTag EventTag = Payload.EventTag;

    if (EventTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Combo.Start"))))
    {
        bComboWindowOpened = true;
        bNextComboReserved = false; 
    }
    else if (EventTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Combo.End"))))
    {
        bComboWindowOpened = false;
    }
    else if (EventTag.MatchesTag(HitCheckEventTag))
    {
        PerformHitCheck();
    }
}

void UGA_Attack::PlayComboSection()
{
    if (!ComboMontage) return;

    FName SectionName = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex);
    
    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("AttackTask"), ComboMontage, 1.0f, SectionName);
    MontageTask->OnCompleted.AddDynamic(this, &UGA_Attack::OnMontageCompleted);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_Attack::OnMontageCompleted);
    MontageTask->ReadyForActivation();
}

void UGA_Attack::PerformHitCheck()
{
    AActor* MyAvatar = GetAvatarActorFromActorInfo();
    UAbilitySystemComponent* MyASC = GetAvatarActorFromActorInfo() ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetAvatarActorFromActorInfo()) : nullptr;
    ACharacter* MyCharacter = Cast<ACharacter>(MyAvatar);

    if (!MyAvatar || !MyASC || !MyCharacter) return;

    int32 ActualComboIndex = CurrentComboIndex;
    UAnimInstance* AnimInstance = MyCharacter->GetMesh()->GetAnimInstance();
    if (AnimInstance)
    {
        FName CurrentSectionName = AnimInstance->Montage_GetCurrentSection(ComboMontage);
        FString SectionString = CurrentSectionName.ToString();

        if (SectionString.Contains(TEXT("1"))) ActualComboIndex = 1;
        else if (SectionString.Contains(TEXT("2"))) ActualComboIndex = 2;
        else if (SectionString.Contains(TEXT("3"))) ActualComboIndex = 3;
    }

    FGameplayCueParameters SwingParams;
    SwingParams.Instigator = MyAvatar;
    SwingParams.TargetAttachComponent = MyCharacter->GetMesh();

    FGameplayTag AttackVFXTag = (ActualComboIndex == 3) ? 
        FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.VFX.Attack.Large")) : 
        FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.VFX.Attack.Small"));
    
    MyASC->ExecuteGameplayCue(AttackVFXTag, SwingParams);

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

    if (ClosestTarget)
    {
        FGameplayCueParameters HitParams;
        HitParams.Instigator = MyAvatar;
        HitParams.EffectCauser = MyAvatar;
        HitParams.Location = ClosestTarget->GetActorLocation();

        MyASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.VFX.Hit")), HitParams);

        FGameplayTag HitSFXTag = (ActualComboIndex == 3) ? 
            FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.SFX.Attack.Large")) : 
            FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.SFX.Attack.Small"));
        MyASC->ExecuteGameplayCue(HitSFXTag, HitParams);

        MyASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.CameraShake.Hit")), HitParams);

        AGasCharacterBase* TargetCharacter = Cast<AGasCharacterBase>(ClosestTarget);
        if (TargetCharacter && MyASC)
        {
            FFuriDamageInfo DamageInfo;
            DamageInfo.Amount = (ActualComboIndex == 3) ? -30.f : -10.f;
            DamageInfo.DamageType = EFuriDamageType::Melee;
            DamageInfo.DamageResponse = (ActualComboIndex == 3) ? EFuriDamageResponse::KnockBack : EFuriDamageResponse::HitReaction;
            DamageInfo.bCanBeParried = (ActualComboIndex < 3); 
            DamageInfo.bCanBeBlocked = true;
            DamageInfo.bShouldForceInterrupt = (ActualComboIndex == 3);

            TSubclassOf<UGameplayEffect> DamageGE = ComboDamageMap.Contains(ActualComboIndex) ? ComboDamageMap[ActualComboIndex] : nullptr;
            
            if (DamageGE)
            {
                // [수정] 기본 MakeEffectContext는 FFuriGameplayEffectContext를 반환하지 않으므로 직접 생성합니다.
                FGameplayEffectContextHandle ContextHandle = FGameplayEffectContextHandle(new FFuriGameplayEffectContext());
                ContextHandle.AddInstigator(MyAvatar, MyAvatar);
                
                FFuriGameplayEffectContext* FuriContext = FFuriGameplayEffectContext::GetFuriContext(ContextHandle);
                if (FuriContext)
                {
                    FuriContext->SetDamageInfo(DamageInfo);
                }

                FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(DamageGE, 1.0f, ContextHandle);
                if (SpecHandle.IsValid())
                {
                    SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")), DamageInfo.Amount);
                    TargetCharacter->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
                }
            }
        }
    }
    else
    {
        PlaySwingSound();
    }

#if !UE_BUILD_SHIPPING 
    FColor DebugColor = (ClosestTarget != nullptr) ? FColor::Red : FColor::Green;
    DrawDebugBox(GetWorld(), BoxCenter, BoxHalfExtent, AvatarRotation.Quaternion(), DebugColor, false, 1.0f, 0, 2.0f);
#endif
}

void UGA_Attack::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Attack::RotateTowardsClosestEnemy(AActor* MyAvatar, float SearchRadius)
{
    if (!MyAvatar || !GetWorld()) return;
    FVector AvatarLocation = MyAvatar->GetActorLocation();
    TArray<FOverlapResult> OverlapResults;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(MyAvatar);
    FCollisionShape SphereShape = FCollisionShape::MakeSphere(SearchRadius + 3000);

    bool bOverlapHit = GetWorld()->OverlapMultiByChannel(OverlapResults, AvatarLocation, FQuat::Identity, ECC_Pawn, SphereShape, Params);

    AActor* ClosestEnemy = nullptr;
    float ClosestDistanceSq = FMath::Square(SearchRadius + 3000.0f);

    if (bOverlapHit)
    {
        for (const FOverlapResult& Result : OverlapResults)
            if (AActor* Actor = Result.GetActor(); Actor && Actor->IsA<APawn>())
            {
                float DistSq = FVector::DistSquared(AvatarLocation, Actor->GetActorLocation());
                if (DistSq < ClosestDistanceSq) { ClosestDistanceSq = DistSq; ClosestEnemy = Actor; }
            }
    }

    if (ClosestEnemy)
    {
        FVector TargetLocation = ClosestEnemy->GetActorLocation();
        TargetLocation.Z = AvatarLocation.Z; 
        FRotator LookAtRotation = FRotationMatrix::MakeFromX(TargetLocation - AvatarLocation).Rotator();
        MyAvatar->SetActorRotation(LookAtRotation);
    }
}

void UGA_Attack::PlaySwingSound()
{
    if (SwingSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, SwingSound, GetAvatarActorFromActorInfo()->GetActorLocation());
    }
}
