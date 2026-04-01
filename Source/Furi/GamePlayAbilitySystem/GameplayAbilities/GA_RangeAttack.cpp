#include "GA_RangeAttack.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Engine/OverlapResult.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/GamePlayAbilitySystem/FuriAbilityTypes.h"
#include "Furi/utils/FuriTypes.h"

UGA_RangeAttack::UGA_RangeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_RangeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      const FGameplayEventData* TriggerEventData)
{
	// 🌟 부모 클래스(UFuriGameplayAbilityBase)의 코스트/쿨타임 적용 기능 작동
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();

	if (MyASC && StartCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = GetAvatarActorFromActorInfo()->GetActorLocation();
		Params.Instigator = GetAvatarActorFromActorInfo();
		Params.NormalizedMagnitude = 3.f;
		MyASC->ExecuteGameplayCue(StartCueTag, Params);
	}

	// 노티파이(GameplayEvent) 대기 태스크
	if (HitEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, HitEventTag);
		if (WaitEventTask)
		{
			WaitEventTask->EventReceived.AddDynamic(this, &UGA_RangeAttack::OnHitEventReceived);
			WaitEventTask->ReadyForActivation();
		}
	}

	// 몽타주 재생
	if (RangeAttackMontage)
	{
		UAbilityTask_PlayMontageAndWait* PlayMontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, TEXT("RangeAttack"), RangeAttackMontage);
		if (PlayMontageTask)
		{
			PlayMontageTask->OnCompleted.AddDynamic(this, &UGA_RangeAttack::OnMontageFinished);
			PlayMontageTask->OnInterrupted.AddDynamic(this, &UGA_RangeAttack::OnMontageFinished);
			PlayMontageTask->OnCancelled.AddDynamic(this, &UGA_RangeAttack::OnMontageFinished);
			PlayMontageTask->ReadyForActivation();
		}
	}
}

void UGA_RangeAttack::OnHitEventReceived(FGameplayEventData Payload)
{
	AActor* MyAvatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
	if (!MyAvatar || !MyASC)
	{
		return;
	}

	FVector Origin = MyAvatar->GetActorLocation();

	// 1. 타격 시점 Cue 실행
	if (HitCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = Origin;
		CueParams.RawMagnitude = AttackRadius;
		CueParams.NormalizedMagnitude = 1.5f;
		MyASC->ExecuteGameplayCue(HitCueTag, CueParams);
	}

#if !UE_BUILD_SHIPPING
	DrawDebugSphere(GetWorld(), Origin, AttackRadius, 32, FColor::Red, false, 1.0f);
#endif

	// 2. 콜리전 스캔
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(MyAvatar);

	bool bHit = GetWorld()->OverlapMultiByChannel(OverlapResults, Origin, FQuat::Identity, ECC_Pawn,
	                                              FCollisionShape::MakeSphere(AttackRadius), Params);

	if (bHit)
	{
		// 🌟 중복 타격 방지용 배열: 한 번 데미지를 준 적은 다시 때리지 않습니다.
		TArray<AActor*> HitActors;

		// 🌟 Data Asset에서 스킬 정보를 가져옵니다.
		FFuriSkillData SkillData;
		bool bHasData = GetCurrentSkillData(SkillData);

		for (const FOverlapResult& Result : OverlapResults)
		{
			AGasCharacterBase* TargetCharacter = Cast<AGasCharacterBase>(Result.GetActor());

			// 유효한 적이고, 아직 때리지 않은 대상이라면
			if (TargetCharacter && !HitActors.Contains(TargetCharacter) && BaseDamageEffectClass)
			{
				HitActors.Add(TargetCharacter); // 타격 명단에 추가

				// DataAsset이 있으면 그 수치를, 없으면 기본 폴백(Fallback) 수치를 사용합니다.
				FFuriDamageInfo FinalDamageInfo;
				if (bHasData)
				{
					FinalDamageInfo = SkillData.DamageInfo;
				}
				else
				{
					// 안전장치: 데이터 에셋을 깜빡했을 경우의 기본값
					FinalDamageInfo.Amount = -50.f;
					FinalDamageInfo.DamageType = EFuriDamageType::Melee;
					FinalDamageInfo.DamageResponse = EFuriDamageResponse::KnockBack;
					FinalDamageInfo.bCanBeParried = false;
					FinalDamageInfo.bCanBeBlocked = true;
					FinalDamageInfo.bShouldForceInterrupt = true;
				}

				// GE Spec 작성 및 적용
				FGameplayEffectContextHandle ContextHandle = MyASC->MakeEffectContext();
				ContextHandle.AddInstigator(MyAvatar, MyAvatar);

				if (FFuriGameplayEffectContext* FuriContext = FFuriGameplayEffectContext::GetFuriContext(ContextHandle))
				{
					FuriContext->SetDamageInfo(FinalDamageInfo);
				}

				FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(
					BaseDamageEffectClass, 1.0f, ContextHandle);
				if (SpecHandle.IsValid())
				{
					// SetByCaller 주입 (음수로 전환)
					SpecHandle.Data.Get()->SetSetByCallerMagnitude(
						FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")), -FinalDamageInfo.Amount);

					// 🌟 ToTarget 사용: 타겟의 ASC에 나의 공격 효과를 넘겨줍니다.
					MyASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(),
					                                       TargetCharacter->GetAbilitySystemComponent());
				}

				// 카메라 셰이크
				FGameplayCueParameters HitParams;
				HitParams.Instigator = MyAvatar;
				HitParams.EffectCauser = MyAvatar;
				HitParams.Location = TargetCharacter->GetActorLocation();
				MyASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.CameraShake.Hit")),
				                          HitParams);
			}
		}
	}
}

void UGA_RangeAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                                 bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_RangeAttack::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
