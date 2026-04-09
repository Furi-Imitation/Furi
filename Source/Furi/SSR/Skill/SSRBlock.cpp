// Fill out your copyright notice in the Description page of Project Settings.

#include "SSRBlock.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectRemoved.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

USSRBlock::USSRBlock()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 🌟 [수정] 방어 상태 태그는 BlockEffectClass(GE)를 통해 부여하는 것이 표준입니다.
	// ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Lock")));
}

void USSRBlock::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                const FGameplayAbilityActivationInfo ActivationInfo,
                                const FGameplayEventData* TriggerEventData)
{
	// 🌟 [수정] 코스트/쿨타임 검사 (GA_Block과 동일하게 수정)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bIsEnding = false;
	bBlockTriggered = false;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (!ASC || !AvatarActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 방어 상태(GE) 적용 - GA_Block 방식 적용
	if (BlockEffectClass)
	{
		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddInstigator(AvatarActor, AvatarActor);

		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(BlockEffectClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			ActiveBlockEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	// 2. Gameplay Cue 실행
	FGameplayCueParameters Params;
	Params.Location = AvatarActor->GetActorLocation();
	Params.Instigator = AvatarActor;

	if (BlockVisualCueTag.IsValid()) { ASC->AddGameplayCue(BlockVisualCueTag); }
	if (BlockStartCueTag.IsValid()) { ASC->ExecuteGameplayCue(BlockStartCueTag, Params); }

	// 3. 애니메이션 몽타주 재생
	if (BlockMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("BlockAni"), BlockMontage);
		if (MontageTask)
		{
			MontageTask->ReadyForActivation();
		}
	}

	// 4. 버튼 떼기 대기 (WaitInputRelease)
	UAbilityTask_WaitInputRelease* InputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this);
	if (InputReleaseTask)
	{
		InputReleaseTask->OnRelease.AddDynamic(this, &USSRBlock::OnInputReleased);
		InputReleaseTask->ReadyForActivation();
	}

	// 6. 🌟 GE가 외부에서 제거되었을 때를 위한 대기 (GA_Block 방식)
	if (ActiveBlockEffectHandle.IsValid())
	{
		UAbilityTask_WaitGameplayEffectRemoved* WaitEffectTask =
			UAbilityTask_WaitGameplayEffectRemoved::WaitForGameplayEffectRemoved(this, ActiveBlockEffectHandle);
		if (WaitEffectTask)
		{
			WaitEffectTask->OnRemoved.AddDynamic(this, &USSRBlock::OnBlockEffectRemoved);
			WaitEffectTask->ReadyForActivation();
		}
	}
}

void USSRBlock::OnInputReleased(float TimeHeld)
{
	// 버튼을 떼면 방어 종료
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USSRBlock::OnBlockEffectRemoved(const FGameplayEffectRemovalInfo& InGameplayEffectRemovalInfo)
{
	// 방어 GE가 사라졌을 때 (스태미나 부족 등) 방어 종료
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USSRBlock::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                           const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                           bool bWasCancelled)
{
	if (bIsEnding)
	{
		return;
	}
	bIsEnding = true;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (ASC && AvatarActor)
	{
		// 1. Gameplay Cue 제거 및 종료 큐 실행
		if (BlockVisualCueTag.IsValid()) { ASC->RemoveGameplayCue(BlockVisualCueTag); }
		if (BlockEndCueTag.IsValid())
		{
			FGameplayCueParameters Params;
			Params.Location = AvatarActor->GetActorLocation();
			Params.Instigator = AvatarActor;
			ASC->ExecuteGameplayCue(BlockEndCueTag, Params);
		}

		// 2. 몽타주 중지
		if (BlockMontage)
		{
			ASC->CurrentMontageStop(0.25f);
		}

		// 3. 🌟 방어 GE가 남아있다면 강제 제거
		if (ActiveBlockEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(ActiveBlockEffectHandle);
			ActiveBlockEffectHandle.Invalidate();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
