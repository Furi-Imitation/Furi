#include "GA_Ultimate.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Furi/FuriPlayerController.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/utils/FuriTypes.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UGA_Ultimate::UGA_Ultimate()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Lock")));
    
    // 기본 태그 세팅
    HitEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Montage.Hit"));
}

void UGA_Ultimate::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) return;

    bFirstHitSuccess = false;
    CurrentHitCount = 0;

    ACharacter* MyChar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    
    // 1. 🚀 [돌진] 캐릭터를 전방으로 발사
    if (MyChar)
    {
        FVector DashDir = MyChar->GetActorForwardVector();
        MyChar->LaunchCharacter(DashDir * DashForce, true, false);
    }

    // 2. 🎯 [이벤트 대기] 모든 타격(1, 2, 3타)은 이 이벤트를 통해 들어옵니다.
    UAbilityTask_WaitGameplayEvent* WaitHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitEventTag);
    WaitHitTask->EventReceived.AddDynamic(this, &UGA_Ultimate::OnHitEventReceived);
    WaitHitTask->ReadyForActivation();

    // 3. 🎬 [몽타주 시작] Dash 섹션부터 시작
    UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("UltTask"), UltimateMontage, 1.0f, TEXT("Dash"));
    PlayTask->OnCompleted.AddDynamic(this, &UGA_Ultimate::OnMontageFinished);
    PlayTask->ReadyForActivation();
}

void UGA_Ultimate::OnHitEventReceived(FGameplayEventData Payload)
{
    CurrentHitCount++; // 타수 카운트 증가

    // 🌟 [1타 적중 시 특수 연출]
    if (CurrentHitCount == 1 && !bFirstHitSuccess)
    {
        bFirstHitSuccess = true;
        
        // 카메라 전환 (PC 호출)
        if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(CurrentActorInfo->PlayerController.Get()))
        {
            if (Payload.Target)
            {
                AActor* TargetActor = const_cast<AActor*>(Payload.Target.Get());
                PC->SetCinematicMode(true, TargetActor);
            }
        }

        // 히트스탑 연출 (0.1초간 느려짐)
        UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.1f);
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([this]() {
            UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
        }), 0.05f, false);

        // [난무] 섹션으로 점프
        MontageJumpToSection(TEXT("Cinematic_Combo"));
    }

    // 🌟 [공통] 데미지 및 Cue 적용 (모든 타격 공통)
    ApplyUltimateDamage(Payload);
}

void UGA_Ultimate::ApplyUltimateDamage(FGameplayEventData Payload)
{
    AActor* MyAvatar = GetAvatarActorFromActorInfo();
    UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
    
    if (!MyAvatar || !MyASC || !Payload.Target) return;

    AGasCharacterBase* TargetChar = Cast<AGasCharacterBase>(const_cast<AActor*>(Payload.Target.Get()));
    if (TargetChar && DamageEffectClass)
    {
        // 1. 데미지 정보 (타수에 따라 대미지 강화 가능)
        FFuriDamageInfo DamageInfo;
        DamageInfo.Amount = (CurrentHitCount == 3) ? -50.f : -15.f; // 막타는 더 강력하게!
        DamageInfo.DamageType = EFuriDamageType::Melee;
        DamageInfo.DamageResponse = (CurrentHitCount == 3) ? EFuriDamageResponse::KnockBack : EFuriDamageResponse::HitReaction;
        DamageInfo.bCanBeParried = false; // 궁극기는 패링 불가

        // 2. GE 적용
        FGameplayEffectContextHandle Context = MyASC->MakeEffectContext();
        FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, Context);
        
        if (SpecHandle.IsValid())
        {
            SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")), DamageInfo.Amount);
            TargetChar->TakeFuriDamage(DamageInfo, SpecHandle, MyAvatar);
            
            // 3. 🎆 [Gameplay Cue] 타격 이펙트 및 카메라 쉐이크
            FGameplayCueParameters CueParams;
            CueParams.Location = TargetChar->GetActorLocation();
            CueParams.Instigator = MyAvatar;
            
            MyASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.VFX.Hit")), CueParams);
            MyASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.CameraShake.Hit")), CueParams);
        }
    }
}

void UGA_Ultimate::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    // 카메라 원래대로 복구
    if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(CurrentActorInfo->PlayerController.Get()))
    {
        PC->SetCinematicMode(false);
    }
    
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}