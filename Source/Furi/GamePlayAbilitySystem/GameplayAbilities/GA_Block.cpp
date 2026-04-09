#include "GA_Block.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectRemoved.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_Block::UGA_Block()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	bIsEnding = false;
}

void UGA_Block::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                const FGameplayAbilityActivationInfo ActivationInfo,
                                const FGameplayEventData* TriggerEventData)
{
	// 🌟 Data Asset 시스템과 연동: 스태미나 소모 및 쿨타임 자동 적용
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bIsEnding = false;

	// 1. 방어 상태(GE) 적용
	if (BlockEffectClass)
	{
		FGameplayEffectContextHandle ContextHandle = ActorInfo->AbilitySystemComponent->MakeEffectContext();
		ContextHandle.AddInstigator(ActorInfo->AvatarActor.Get(), ActorInfo->AvatarActor.Get());

		FGameplayEffectSpecHandle SpecHandle = ActorInfo->AbilitySystemComponent->MakeOutgoingSpec(
			BlockEffectClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			// 방어 이펙트를 부여하고 그 영수증(Handle)을 보관합니다.
			ActiveBlockEffectHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(
				*SpecHandle.Data.Get());
		}
	}
	FGameplayCueParameters Params;
	Params.Location = ActorInfo->AvatarActor->GetActorLocation();
	Params.Instigator = ActorInfo->AvatarActor.Get();
	// 2. 방어 시작 Gameplay Cue (이펙트 & 사운드)
	if (BlockStartCueTag.IsValid())
	{
		ActorInfo->AbilitySystemComponent->ExecuteGameplayCue(BlockStartCueTag, Params);
	}

	ActorInfo->AbilitySystemComponent->ExecuteGameplayCue(
		FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.SFX.Block")), Params);

	// 3. 방어 애니메이션 재생
	if (BlockMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("BlockAni"), BlockMontage);
		if (MontageTask)
		{
			// 🌟 [추가] 몽타주 종료/중단/취소 시 어빌리티 종료 처리 (안전장치)
			MontageTask->OnCompleted.AddDynamic(this, &UGA_Block::OnBlockMontageFinished);
			MontageTask->OnInterrupted.AddDynamic(this, &UGA_Block::OnBlockMontageFinished);
			MontageTask->OnCancelled.AddDynamic(this, &UGA_Block::OnBlockMontageFinished);
			MontageTask->ReadyForActivation();
		}
	}

	// 4. GE가 만료되거나 파괴될 때까지 대기
	if (ActiveBlockEffectHandle.IsValid())
	{
		UAbilityTask_WaitGameplayEffectRemoved* WaitEffectTask =
			UAbilityTask_WaitGameplayEffectRemoved::WaitForGameplayEffectRemoved(this, ActiveBlockEffectHandle);
		if (WaitEffectTask)
		{
			WaitEffectTask->OnRemoved.AddDynamic(this, &UGA_Block::OnBlockEffectRemoved);
			WaitEffectTask->ReadyForActivation();
		}
	}
}

void UGA_Block::OnBlockEffectRemoved(const FGameplayEffectRemovalInfo& InGameplayEffectRemovalInfo)
{
	// 방어 이펙트가 (시간이 다 되어서, 혹은 적에게 강제로 깨져서) 사라졌다면 스킬을 종료합니다.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Block::OnBlockMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Block::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                           const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                           bool bWasCancelled)
{
	// 🌟 무한 루프 방지 (이미 종료 중이면 무시)
	if (bIsEnding)
	{
		return;
	}
	bIsEnding = true;

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		// 1. 방어 종료 Gameplay Cue
		if (BlockEndCueTag.IsValid())
		{
			FGameplayCueParameters Params;
			Params.Location = ActorInfo->AvatarActor->GetActorLocation();
			Params.Instigator = ActorInfo->AvatarActor.Get();
			ActorInfo->AbilitySystemComponent->ExecuteGameplayCue(BlockEndCueTag, Params);
		}

		// 2. 🌟 애니메이션 정지는 한 번만! (0.25초 블렌드 아웃으로 부드럽게 복구)
		if (BlockMontage)
		{
			ActorInfo->AbilitySystemComponent->CurrentMontageStop(0.25f);
		}

		// 3. 만약 스킬이 수동으로 취소되었거나 끝났는데 GE가 남아있다면 강제로 제거합니다.
		if (ActiveBlockEffectHandle.IsValid())
		{
			ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveBlockEffectHandle);
			ActiveBlockEffectHandle.Invalidate(); // 영수증 파기
		}
	}

	// 부모 클래스 호출 (항상 맨 마지막에 위치)
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
