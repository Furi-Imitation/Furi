// Fill out your copyright notice in the Description page of Project Settings.


#include "SSRDash.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"

USSRDash::USSRDash()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 1. 다른 동작 못하게 하기 (Ability Tag & Block Tags)
	// 이 어빌리티의 식별 태그
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Action.SSRDash")));
    
	// 대쉬 중에는 '공격'이나 '다른 스킬' 태그가 붙은 어빌리티가 실행되지 못하도록 차단합니다.
	BlockAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Action"))); 
    
	// 2. 무적 및 상태 태그 (Activation Owned Tags)
	// 어빌리티가 실행되는 동안 캐릭터에게 자동으로 부여되는 태그입니다.
	// 'State.Invulnerable' 태그가 있으면 데미지 계산 로직에서 무시하도록 설정할 수 있습니다.
	// dash 없애고 invisi
	// ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Dash")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Invincible"))); // 무적 태그
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Lock")));         // 이동/회전 제한용 태그
}

void USSRDash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogTemp, Warning, TEXT("SSRDash: ActivateAbility Started!"));
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	// Cue 설정
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	
	if (!Character || !ASC) return;

	// 1. 캐릭터 모습 숨기기 (사라지는 연출) / 블프로 함
	// Character->GetMesh()->SetHiddenInGame(true, true);
	
	if (DashStartCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = Character->GetActorLocation();
		Params.Instigator = Character;
		ASC->AddGameplayCue(DashStartCueTag, Params);
	}

	// 2. 방향 결정 (입력 방향 vs 캐릭터 정면)
	FVector DashDir = GetDashDirection();

	// 3. 루트 모션 태스크 실행 (물리적인 대쉬 이동)
	UAbilityTask_ApplyRootMotionConstantForce* DashTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this, 
		TEXT("DashTask"), 
		DashDir, 
		DashStrength, 
		DashDuration, 
		false,      // IsAdditive (기존 속도에 더할지 여부)
		nullptr,    // StrengthOverTimeCurve
		ERootMotionFinishVelocityMode::SetVelocity, 
		FVector::ZeroVector, 
		0.f, 
		false
	);

	if (DashTask)
	{
		DashTask->OnFinish.AddDynamic(this, &USSRDash::OnDashFinished);
		DashTask->ReadyForActivation(); // 실행할 준비
	}
}

void USSRDash::OnDashFinished()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (Character)
	{
		// 4. 대쉬 종료 시 캐릭터 다시 보이기 / 블프로 함
		// Character->GetMesh()->SetHiddenInGame(false, true);
		
		// if(DashEndCueTag.IsValid() && ASC)
		// {
		// 	FGameplayCueParameters Params;
		// 	Params.Location = Character->GetActorLocation();
		// 	Params.Instigator = Character;
		// 	ASC->ExecuteGameplayCue(DashEndCueTag, Params);
		// }
		
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

FVector USSRDash::GetDashDirection()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character) return FVector::ForwardVector;

	// 현재 플레이어의 입력 방향 벡터를 가져옵니다.
	FVector InputVector = Character->GetLastMovementInputVector();

	// 입력이 거의 없다면 (정지 상태라면) 캐릭터가 바라보는 정면 사용
	if (InputVector.IsNearlyZero())
	{
		return Character->GetActorForwardVector();
	}

	// 입력이 있다면 누르고 있는 방향을 사용 (정규화 필수)
	return InputVector.GetSafeNormal();
}

void USSRDash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if(DashStartCueTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("✨ Removing Dash Cue"));
		ASC->RemoveGameplayCue(DashStartCueTag);
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

