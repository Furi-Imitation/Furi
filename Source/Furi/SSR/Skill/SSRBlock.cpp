// Fill out your copyright notice in the Description page of Project Settings.


#include "SSRBlock.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

USSRBlock::USSRBlock()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 네트워크 예측 설정 (로컬에서 즉시 반응)
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    
	// 방어 중에는 이동 방지 등의 태그 설정 가능
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Lock")));
}

void USSRBlock::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogTemp, Warning, TEXT("=== SSRBlock Start ==="));
	
	UE_LOG(LogTemp, Warning, TEXT("Attempting to Commit..."));
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Error, TEXT("❌ CommitAbility FAILED! (Cost or Cooldown)"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("✅ CommitAbility SUCCESS!"));
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	bBlockTriggered = false;
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* AvatarActor = GetAvatarActorFromActorInfo(); // 내 캐릭터 가져오기
	
	// 블락중 지속될 파란 구체 이펙트
	if (ASC && AvatarActor && BlockVisualCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Instigator = AvatarActor;
		Params.EffectCauser = AvatarActor;
		// 소유자를 명시적으로 전달하거나 ASC를 통해 대상을 지정합니다
		ASC->AddGameplayCue(BlockVisualCueTag);
	}
	
	// VFX/SFX 시작 큐
	if (BlockStartCueTag.IsValid())
	{
		ASC->ExecuteGameplayCue(BlockStartCueTag, FGameplayCueParameters());
	}
	
	// 애니메이션 몽타쥬
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("Block"), BlockMontage);
	MontageTask->ReadyForActivation();
	
	// 이벤트 방어 성공 이벤트 대기
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(FName("Event.Hit.Block")));
	EventTask->EventReceived.AddDynamic(this, &USSRBlock::OnBlockSuccess);
	EventTask->ReadyForActivation();
	
	// Timer 1초 타임아웃 대기
	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, BlockMaxDuration);
	DelayTask->OnFinish.AddDynamic(this, &USSRBlock::OnBlockTimeout);
	DelayTask->ReadyForActivation();
}


void USSRBlock::OnBlockSuccess(FGameplayEventData Payload)
{
	if (bBlockTriggered) return;
	bBlockTriggered = true;
	
	DrawDebugSphere(GetWorld(), Payload.Instigator->GetActorLocation(), 50.f, 12, FColor::Red, false, 1.0f);
	
	// 적(공격자)에게 기절 효과 부여
	// Payload.Instigator는 나를 때린 '적'입니다.
	if (StunEffectClass)
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Payload.Instigator);
		if (TargetASC)
		{
			FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
			Context.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());
			
			// 기절 이펙트 적용
			FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(StunEffectClass, 1.0f, Context);
			if (SpecHandle.IsValid())
			{
				TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
	
	// 성공시 즉시 종료
	EndAbility(CurrentSpecHandle,CurrentActorInfo, CurrentActivationInfo,true, false);
}

void USSRBlock::OnBlockTimeout()
{
	// 이미 방어에 성공했으면 리턴
	if (bBlockTriggered) return;
	
	EndAbility(CurrentSpecHandle,CurrentActorInfo, CurrentActivationInfo,true, false);
}

void USSRBlock::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	
	// [추가] 블락 종료 시 구체 이펙트 제거 (Remove)
	if (BlockVisualCueTag.IsValid() && ASC)
	{
		ASC->RemoveGameplayCue(BlockVisualCueTag);
	}
	
	if (BlockEndCueTag.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(BlockEndCueTag, FGameplayCueParameters());
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
