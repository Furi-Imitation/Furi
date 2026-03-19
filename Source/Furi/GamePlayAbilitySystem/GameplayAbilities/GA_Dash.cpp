#include "GA_Dash.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameplayTask.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Dash::UGA_Dash()
{
	// 대시 중에는 다른 이동 입력을 무시하거나 특정 상태임을 알리는 태그
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Action.Dash")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Dashing")));
}

void UGA_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character) { EndAbility(Handle, ActorInfo, ActivationInfo, true, false); return; }

    // [연출] 대시 시작 시 메시 숨기기
    Character->GetMesh()->SetHiddenInGame(true);
    TArray<USceneComponent*> Children;
    Character->GetMesh()->GetChildrenComponents(true, Children);
    for (USceneComponent* Child : Children) { Child->SetHiddenInGame(true); }

    // 1. 시작할 때 (ActivateAbility 내부)
    if (DashStartCueTag.IsValid()) 
    {
        // [Gameplay Cue] 시작 사운드 및 이펙트 실행 (Tag: GameplayCue.P1.Dash.Start)
        FGameplayCueParameters Params;
        Params.Location = Character->GetActorLocation();
        Params.Instigator = Character;
        GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(DashStartCueTag, Params);
    }
    
    // [Task] 루트 모션 이동 (0.3초 동안 대시)
    FVector DashDir = Character->GetLastMovementInputVector().IsNearlyZero() ? Character->GetActorForwardVector() : Character->GetLastMovementInputVector();
    UAbilityTask_ApplyRootMotionConstantForce* MoveTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
        this, TEXT("DashMove"), DashDir, 2500.f, 0.3f, false, nullptr, ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, 600.f, false);
    
    // [Task] 애니메이션 재생
    UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("DashAni"), DashMontage);

    if (MoveTask && MontageTask) {
        MoveTask->OnFinish.AddDynamic(this, &UGA_Dash::OnDashFinished);
        MoveTask->ReadyForActivation();
        MontageTask->ReadyForActivation();
    }
}

void UGA_Dash::OnDashFinished() {
    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Character) {
        Character->GetMesh()->SetHiddenInGame(false); // 메시 다시 보이기
        TArray<USceneComponent*> Children;
        Character->GetMesh()->GetChildrenComponents(true, Children);
        for (USceneComponent* Child : Children) { Child->SetHiddenInGame(false); }
        
        // [Gameplay Cue] 종료 사운드 및 이펙트 실행 (Tag: GameplayCue.P1.Dash.End)
        if (DashEndCueTag.IsValid())
        {
            FGameplayCueParameters Params;
            Params.Location = Character->GetActorLocation(); // 현재 위치 (도착지)
            Params.Instigator = Character;
            GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(DashEndCueTag, Params);
        }
    }
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}