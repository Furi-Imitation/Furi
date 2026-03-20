// GA_Dash.cpp

#include "GA_Dash.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"

UGA_Dash::UGA_Dash()
{
    // [설정] 이 능력이 실행 중일 때 캐릭터가 가질 태그 (예: 대시 중엔 점프 불가 등을 판단할 때 사용)
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Action.Dash")));
    
    // [네트워크] 로컬 예측(Local Predicted): 클라이언트가 서버 응답을 기다리지 않고 즉시 대시를 시작하여 조작감을 높입니다.
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    
    // [상태 태그] 능력이 활성화된 동안 캐릭터에게 부여되는 태그 (애니메이션 블루프린트 등에서 참조 가능)
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Dashing")));
}

void UGA_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    // 1. [검증] 코스트(마나 등)와 쿨타임이 충족되었는지 확인하고 소모합니다.
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character) { EndAbility(Handle, ActorInfo, ActivationInfo, true, false); return; }

    // 2. [연출: 은신 효과] 대시 시작 시 본체와 자식 컴포넌트(무기 등)를 모두 숨깁니다.
    Character->GetMesh()->SetHiddenInGame(true);
    TArray<USceneComponent*> Children;
    Character->GetMesh()->GetChildrenComponents(true, Children);
    for (USceneComponent* Child : Children) { Child->SetHiddenInGame(true); }

    // 3. [Gameplay Cue] 사운드나 파티클 같은 '시각적 효과'를 실행합니다.
    if (DashStartCueTag.IsValid()) 
    {
        FGameplayCueParameters Params;
        Params.Location = Character->GetActorLocation();
        Params.Instigator = Character;
        GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(DashStartCueTag, Params);
    }
    
    // 4. [이동 로직] 입력 방향이 있으면 그 방향으로, 없으면 앞방향으로 대시 방향을 결정합니다.
    FVector DashDir = Character->GetLastMovementInputVector().IsNearlyZero() ? Character->GetActorForwardVector() : Character->GetLastMovementInputVector();
    
    // 5. [Ability Task] 루트 모션을 사용하여 0.3초 동안 2500의 속도로 밀어내는 '태스크'를 생성합니다.
    UAbilityTask_ApplyRootMotionConstantForce* MoveTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
        this, TEXT("DashMove"), DashDir, 2500.f, 0.3f, false, nullptr, ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, 600.f, false);

    if (MoveTask) 
    {
        // 6. [이벤트 바인딩] 대시 이동이 끝나면 OnDashFinished 함수가 실행되도록 연결합니다.
        MoveTask->OnFinish.AddDynamic(this, &UGA_Dash::OnDashFinished);
        
        // 7. [활성화] 태스크를 실제로 가동시킵니다.
        MoveTask->ReadyForActivation();
    }
    else
    {
        // 실패 시 즉시 종료하여 캐릭터가 굳는 버그 방지
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

void UGA_Dash::OnDashFinished() {
    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Character) {
        // 8. [복구] 숨겼던 메시를 다시 보이게 합니다.
        Character->GetMesh()->SetHiddenInGame(false);
        TArray<USceneComponent*> Children;
        Character->GetMesh()->GetChildrenComponents(true, Children);
        for (USceneComponent* Child : Children) { Child->SetHiddenInGame(false); }
        
        // 9. [Gameplay Cue] 대시 도착 지점 이펙트 실행
        if (DashEndCueTag.IsValid())
        {
            FGameplayCueParameters Params;
            Params.Location = Character->GetActorLocation();
            Params.Instigator = Character;
            GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(DashEndCueTag, Params);
        }
    }
    
    // 10. [능력 종료] 이 함수를 호출해야 'State.Dashing' 태그가 제거되고 다른 능력을 쓸 수 있습니다.
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}