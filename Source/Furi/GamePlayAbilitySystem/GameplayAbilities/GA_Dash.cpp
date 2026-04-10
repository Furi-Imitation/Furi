// GA_Dash.cpp

#include "GA_Dash.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"

UGA_Dash::UGA_Dash()
{
	// [네트워크] 로컬 예측(Local Predicted): 클라이언트가 서버 응답을 기다리지 않고 즉시 대시를 시작하여 조작감을 높입니다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                               const FGameplayAbilityActivationInfo ActivationInfo,
                               const FGameplayEventData* TriggerEventData)
{
	// Data Asset에 적힌 코스트(스태미나)와 쿨타임이 충족되었는지 확인하고 소모합니다.
	// 🌟 우리가 부모 클래스(UFuriGameplayAbilityBase)에서 오버라이드한 로직이 여기서 완벽하게 돌아갑니다!
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 2. [Gameplay Cue] 대시 도중 지속되는 시각 효과 (예: 잔상, 캐릭터 투명화 등)을 켭니다.
	if (DashVisualCueTag.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->AddGameplayCue(DashVisualCueTag);
	}

	// 3. [Gameplay Cue] 대시 시작 시점의 일회성 효과 (사운드, 파티클 터짐)
	if (DashStartCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = Character->GetActorLocation();
		Params.Instigator = Character;
		GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(DashStartCueTag, Params);
	}

	// 4. [이동 로직] 입력 방향이 있으면 그 방향으로, 없으면 앞방향으로 대시 방향을 결정합니다.
	FVector DashDir = Character->GetLastMovementInputVector().IsNearlyZero()
		                  ? Character->GetActorForwardVector()
		                  : Character->GetLastMovementInputVector();

	// 5. [Ability Task] 루트 모션을 사용하여 대시를 수행합니다.
	// 🌟 하드코딩된 2500.f 대신, 기획자가 에디터에서 조절할 수 있도록 DashStrength 변수를 사용합니다.
	UAbilityTask_ApplyRootMotionConstantForce* MoveTask =
		UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
			this, TEXT("DashMove"), DashDir, DashStrength, 0.3f, false, nullptr,
			ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, 600.f, false);

	if (MoveTask)
	{
		// 6. [이벤트 바인딩] 대시 이동이 끝나면 OnDashFinished 함수가 실행되도록 연결합니다.
		MoveTask->OnFinish.AddDynamic(this, &UGA_Dash::OnDashFinished);

		// 7. [활성화] 태스크를 실제로 가동시킵니다.
		MoveTask->ReadyForActivation();
	}
	else
	{
		// 실패 시 즉시 종료하여 캐릭터가 굳는 버그를 방지합니다.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_Dash::OnDashFinished()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character)
	{
		// 🌟 [Gameplay Cue] 도착 이펙트는 정상적으로 이동을 마쳤을 때만 터지도록 합니다.
		if (DashEndCueTag.IsValid())
		{
			FGameplayCueParameters Params;
			Params.Location = Character->GetActorLocation();
			Params.Instigator = Character;
			GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(DashEndCueTag, Params);
		}
	}

	// 어빌리티 정상 종료 호출 (이 함수가 호출되면 아래의 오버라이드된 EndAbility로 넘어갑니다)
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                          const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                          bool bWasCancelled)
{
	// 🌟 대시가 종류별로 끝났거나 취소되었으므로, 켜두었던 잔상 큐를 확실하게 꺼줍니다.
	if (DashVisualCueTag.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveGameplayCue(DashVisualCueTag);
	}

	// 부모 클래스의 EndAbility는 반드시 로직의 맨 마지막에 호출해야 합니다.
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
