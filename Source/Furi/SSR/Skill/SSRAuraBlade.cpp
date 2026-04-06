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

	if (ChargeCueTag.IsValid())
	{
		ASC->AddGameplayCue(ChargeCueTag);
	}

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, DelayTime);
	if (DelayTask)
	{
		DelayTask->OnFinish.AddDynamic(this, &USSRAuraBlade::OnDelayFinished);
		DelayTask->ReadyForActivation();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void USSRAuraBlade::OnDelayFinished()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!Character || !ASC)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (ChargeCueTag.IsValid()) { ASC->RemoveGameplayCue(ChargeCueTag); }
	if (FireCueTag.IsValid()) { ASC->ExecuteGameplayCue(FireCueTag); }

	FGameplayEffectSpecHandle DamageSpecHandle;
	float FinalDamageAmount = 20.0f; // 안전장치 기본값

	// 🌟 [핵심] Data Asset에서 대미지 정보를 동적으로 가져와서 컨텍스트(Context)를 조립합니다.
	FFuriSkillData SkillData;
	if (GetCurrentSkillData(SkillData))
	{
		FFuriDamageInfo DamageInfo = SkillData.DamageInfo;
		FinalDamageAmount = DamageInfo.Amount;

		// BaseDamageEffectClass를 사용하여 Spec 생성
		if (BaseDamageEffectClass)
		{
			// 엔진 표준 방식으로 컨텍스트 생성 (new 방식을 대체)
			FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
			ContextHandle.AddInstigator(Character, Character);

			// 커스텀 컨텍스트에 정보 심기
			if (FFuriGameplayEffectContext* FuriContext = FFuriGameplayEffectContext::GetFuriContext(ContextHandle))
			{
				FuriContext->SetDamageInfo(DamageInfo);
			}

			DamageSpecHandle = MakeOutgoingGameplayEffectSpec(BaseDamageEffectClass, GetAbilityLevel());

			if (DamageSpecHandle.IsValid())
			{
				// 생성된 Spec에 컨텍스트를 덮어씌우고, SetByCaller 주입 (음수)
				DamageSpecHandle.Data.Get()->SetContext(ContextHandle);
				DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(
					FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")), -FinalDamageAmount);
			}
		}
	}

	// 투사체 소환
	if (ProjectileClass)
	{
		FVector ForwardVector = Character->GetActorForwardVector();
		FVector SpawnLocation = Character->GetActorLocation() + (ForwardVector * 100.f) + FVector(0.f, 0.f, 50.f);
		FRotator SpawnRotation = Character->GetActorRotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AAuraBladeProjectile* Projectile = GetWorld()->SpawnActor<AAuraBladeProjectile>(
			ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

		if (Projectile)
		{
			// 🌟 Data Asset에서 가져온 실제 대미지와 완벽하게 조립된 Spec 전달
			Projectile->Initialize(FinalDamageAmount, 1.0f, DamageSpecHandle, HitCueTag);
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
