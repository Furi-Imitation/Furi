#include "SSRAuraBlade.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Furi/SSR/AuraBladeProjectile.h"
#include "GameFramework/Character.h"
#include "Furi/GamePlayAbilitySystem/FuriAbilityTypes.h" // 🌟 커스텀 컨텍스트를 위해 추가

USSRAuraBlade::USSRAuraBlade()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 🚨 [수정] CDO 크래시 주범 제거! 블루프린트 디테일 패널의 'Activation Owned Tags'에 State.SkillUsing을 추가하세요.
	// ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.SkillUsing")));
}

void USSRAuraBlade::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                    const FGameplayAbilityActivationInfo ActivationInfo,
                                    const FGameplayEventData* TriggerEventData)
{
	
	
	// 🌟 [수정] 부모 클래스의 스태미나 코스트 및 쿨타임 결제 확인 (최상단 배치)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	// 🌟 추가: 버튼을 떼도 어빌리티가 취소되지 않도록 강제 설정
	bRetriggerInstancedAbility = false;

	if (ChargeMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, ChargeMontage);
		MontageTask->ReadyForActivation();
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}
	// 🔍 로그 추가: 서버/클라이언트 양쪽에서 찍히는지 확인
	UE_LOG(LogTemp, Warning, TEXT("ActivateAbility - IsServer: %d"), ASC->IsOwnerActorAuthoritative());
	if (ChargeCueTag.IsValid())
	{
		ASC->AddGameplayCue(ChargeCueTag);
	}
	
	// 🔍 서버에서 DelayTime이 얼마인지 확인!
	UE_LOG(LogTemp, Warning, TEXT("DelayTime Check - IsServer: %d, Value: %f"), 
		   ASC->IsOwnerActorAuthoritative(), DelayTime);

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, DelayTime);
	if (DelayTask)
	{
		DelayTask->OnFinish.AddDynamic(this, &USSRAuraBlade::OnDelayFinished);
		DelayTask->ReadyForActivation();
	}
	else
	{
		// EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void USSRAuraBlade::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogTemp, Error, TEXT("=== SERVER END ABILITY DETECTED ==="));
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USSRAuraBlade::OnDelayFinished()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character) return;

	// 🌟 1. 일단 큐(이펙트)부터 무조건 실행해서 함수가 도는지 확인
	GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(FireCueTag);

	// 🌟 2. 조건문 없이 그냥 스폰 (서버라면 엔진이 알아서 생성하고 복제함)
	if (ProjectileClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FVector SpawnLocation = Character->GetActorLocation() + (Character->GetActorForwardVector() * 150.f) + FVector(0.f, 0.f, 50.f);
        
		// 🔍 로그 레벨을 Error로 올려서 무조건 보이게 합니다.
		UE_LOG(LogTemp, Error, TEXT("--- FORCE SPAWN ATTEMPT ---"));

		GetWorld()->SpawnActor<AAuraBladeProjectile>(ProjectileClass, SpawnLocation, Character->GetActorRotation(), SpawnParams);
	}

	// 🌟 3. 종료를 '지연' 시켜서 리플리케이션 시간을 벌어줍니다 (테스트용)
	// EndAbility를 호출하지 않거나, 아주 나중에 호출되게 해보세요.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

// void USSRAuraBlade::OnDelayFinished()
// {
// 	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
// 	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
//
// 	if (!Character || !ASC)
// 	{
// 		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
// 		return;
// 	}
//
// 	// 시각 효과는 양쪽 다 실행
// 	if (ChargeCueTag.IsValid()) { ASC->RemoveGameplayCue(ChargeCueTag); }
// 	if (FireCueTag.IsValid()) { ASC->ExecuteGameplayCue(FireCueTag); }
//
// 	// 🌟 [핵심] 아무런 조건문 없이 바로 Spawn 시도
// 	if (ProjectileClass)
// 	{
// 		FVector ForwardVector = Character->GetActorForwardVector();
// 		FVector SpawnLocation = Character->GetActorLocation() + (ForwardVector * 150.f) + FVector(0.f, 0.f, 50.f);
// 		FRotator SpawnRotation = Character->GetActorRotation();
//
// 		FActorSpawnParameters SpawnParams;
// 		SpawnParams.Owner = Character;
// 		SpawnParams.Instigator = Character;
// 		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//
// 		// 서버에서 실행될 때만 액터가 생성되고, 리플리케이션 설정에 의해 클라이언트로 복제됩니다.
// 		AAuraBladeProjectile* Projectile = GetWorld()->SpawnActor<AAuraBladeProjectile>(
// 			ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
//
// 		if (Projectile)
// 		{
// 			UE_LOG(LogTemp, Warning, TEXT("!!! PROJECTILE SPAWNED !!!"));
//             
// 			// 대미지 로직 (최소한의 안전장치)
// 			FGameplayEffectSpecHandle DamageSpecHandle = MakeOutgoingGameplayEffectSpec(BaseDamageEffectClass, GetAbilityLevel());
// 			Projectile->Initialize(20.0f, 1.0f, DamageSpecHandle, HitCueTag);
// 		}
// 	}
//
// 	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
// }
