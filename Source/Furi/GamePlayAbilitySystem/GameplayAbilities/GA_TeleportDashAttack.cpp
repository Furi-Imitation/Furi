#include "GA_TeleportDashAttack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Engine/OverlapResult.h"
#include "Furi/FuriBlueprintFunctionLibrary.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/GamePlayAbilitySystem/FuriAbilityTypes.h"
#include "Furi/utils/FuriTypes.h"

UGA_TeleportDashAttack::UGA_TeleportDashAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_TeleportDashAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo,
                                             const FGameplayAbilityActivationInfo ActivationInfo,
                                             const FGameplayEventData* TriggerEventData)
{
	// 🌟 Data Asset의 스태미나 코스트 및 쿨타임 검사/소모
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CurrentStrikeCount = 0;
	AActor* MyAvatar = GetAvatarActorFromActorInfo();
	TArray<AActor*> IgnoredActors;

	LockedTarget = UFuriBlueprintFunctionLibrary::FindClosestTarget(MyAvatar, 2000.f, IgnoredActors);

	if (!IsValid(LockedTarget))
	{
		UE_LOG(LogTemp, Warning, TEXT("텔레포트 할 타겟이 없습니다!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (HitEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* WaitHitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, HitEventTag);
		if (WaitHitEventTask)
		{
			WaitHitEventTask->EventReceived.AddDynamic(this, &UGA_TeleportDashAttack::OnHitCheckEventReceived);
			WaitHitEventTask->ReadyForActivation();
		}
	}

	PrepareNextStrike();
}

void UGA_TeleportDashAttack::PrepareNextStrike()
{
	ACharacter* MyChar = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	if (!MyChar || !IsValid(LockedTarget) || CurrentStrikeCount >= MaxStrikeCount)
	{
		bool bWasCancelled = (CurrentStrikeCount < MaxStrikeCount);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
		return;
	}


	if (VanishCueTag.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->AddGameplayCue(VanishCueTag);
	}

	if (TeleportCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = MyChar->GetActorLocation();
		Params.Instigator = MyChar;
		GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(TeleportCueTag, Params);
	}

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, StrikeDelay);
	if (DelayTask)
	{
		DelayTask->OnFinish.AddDynamic(this, &UGA_TeleportDashAttack::OnStrikeDelayFinished);
		DelayTask->ReadyForActivation();
	}
}

void UGA_TeleportDashAttack::OnStrikeDelayFinished()
{
	ACharacter* MyChar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();

	if (!MyChar || !MyASC)
	{
		return;
	}

	if (VanishCueTag.IsValid())
	{
		MyASC->RemoveGameplayCue(VanishCueTag);
	}


	FVector TeleportLoc;
	FRotator TeleportRot;
	UFuriBlueprintFunctionLibrary::CalculateTeleportTransform(LockedTarget, CurrentStrikeCount + 1, 350.f, TeleportLoc,
	                                                          TeleportRot);

	MyChar->SetActorLocationAndRotation(TeleportLoc, TeleportRot, false, nullptr, ETeleportType::TeleportPhysics);

	if (TeleportCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = MyChar->GetActorLocation();
		Params.Instigator = MyChar;
		MyASC->ExecuteGameplayCue(TeleportCueTag, Params);
	}

	if (StrikeMontages.IsValidIndex(CurrentStrikeCount) && StrikeMontages[CurrentStrikeCount])
	{
		UAnimMontage* MontageToPlay = StrikeMontages[CurrentStrikeCount];

		UAbilityTask_PlayMontageAndWait* PlayMontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("StrikeMontage"), MontageToPlay);

		if (PlayMontageTask)
		{
			PlayMontageTask->OnCompleted.AddDynamic(this, &UGA_TeleportDashAttack::OnStrikeMontageFinished);
			PlayMontageTask->OnInterrupted.AddDynamic(this, &UGA_TeleportDashAttack::OnStrikeInterrupted);
			PlayMontageTask->OnCancelled.AddDynamic(this, &UGA_TeleportDashAttack::OnStrikeInterrupted);
			PlayMontageTask->ReadyForActivation();
		}
	}

	CurrentStrikeCount++;
}

void UGA_TeleportDashAttack::OnStrikeMontageFinished()
{
	PrepareNextStrike();
}

void UGA_TeleportDashAttack::OnStrikeInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_TeleportDashAttack::OnHitCheckEventReceived(FGameplayEventData Payload)
{
	AActor* MyAvatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();

	if (!MyAvatar || !LockedTarget || !MyASC)
	{
		return;
	}

	FVector Forward = MyAvatar->GetActorForwardVector();
	FVector BoxCenter = MyAvatar->GetActorLocation() + (Forward * (AttackRange * 0.5f));
	FVector BoxHalfExtent = FVector(AttackRange * 0.5f, AttackBoxHalfWidth, AttackBoxHalfHeight);
	FQuat BoxRotation = MyAvatar->GetActorQuat();

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(MyAvatar);

	bool bHit = GetWorld()->OverlapMultiByChannel(OverlapResults, BoxCenter, BoxRotation, ECC_Pawn,
	                                              FCollisionShape::MakeBox(BoxHalfExtent), QueryParams);

#if !UE_BUILD_SHIPPING
	DrawDebugBox(GetWorld(), BoxCenter, BoxHalfExtent, BoxRotation, FColor::Red, false, 1.0f);
#endif

	if (bHit)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			if (Result.GetActor() == LockedTarget)
			{
				AGasCharacterBase* TargetChar = Cast<AGasCharacterBase>(LockedTarget);
				if (TargetChar && BaseDamageEffectClass)
				{
					if (CameraShakeCueTag.IsValid())
					{
						FGameplayCueParameters HitParams;
						HitParams.Instigator = MyAvatar;
						HitParams.EffectCauser = MyAvatar;
						HitParams.Location = TargetChar->GetActorLocation();
						MyASC->ExecuteGameplayCue(CameraShakeCueTag, HitParams);

						FGameplayTag HitSFXTag = (CurrentStrikeCount == 3)
							                         ? FGameplayTag::RequestGameplayTag(
								                         FName("GameplayCue.P1.SFX.Attack.Large"))
							                         : FGameplayTag::RequestGameplayTag(
								                         FName("GameplayCue.P1.SFX.Attack.Small"));
						MyASC->ExecuteGameplayCue(HitSFXTag, HitParams);
					}

					// 🌟 하드코딩 대신 블루프린트에서 설정한 태그 사용
					if (AttackVFXCueTag.IsValid())
					{
						FGameplayCueParameters SwingParams;
						SwingParams.Instigator = TargetChar;
						SwingParams.TargetAttachComponent = TargetChar->GetMesh();
						MyASC->ExecuteGameplayCue(AttackVFXCueTag, SwingParams);
					}

					// 🌟 Data Asset에서 스킬 데이터 로드
					FFuriSkillData SkillData;
					bool bHasData = GetCurrentSkillData(SkillData);

					FFuriDamageInfo FinalDamageInfo;
					if (bHasData)
					{
						FinalDamageInfo = SkillData.DamageInfo;
					}
					else
					{
						// 데이터 에셋 누락 대비 안전장치
						FinalDamageInfo.Amount = 15.f;
						FinalDamageInfo.DamageType = EFuriDamageType::Melee;
					}

					// 🌟 막타(3타)일 경우의 파괴력 증폭 로직
					// (OnStrikeDelayFinished에서 Montage 재생 직전에 ++되므로, 1~3 값을 가집니다)
					if (CurrentStrikeCount == MaxStrikeCount)
					{
						FinalDamageInfo.Amount *= 2.0f; // 막타는 2배의 대미지!
						FinalDamageInfo.DamageResponse = EFuriDamageResponse::KnockBack;
						FinalDamageInfo.bCanBeParried = false;
						FinalDamageInfo.bShouldForceInterrupt = true;
					}
					else
					{
						FinalDamageInfo.DamageResponse = EFuriDamageResponse::Stagger;
						FinalDamageInfo.bCanBeParried = true;
					}

					FGameplayEffectContextHandle ContextHandle = MyASC->MakeEffectContext();
					ContextHandle.AddInstigator(MyAvatar, MyAvatar);

					if (FFuriGameplayEffectContext* FuriContext = FFuriGameplayEffectContext::GetFuriContext(
						ContextHandle))
					{
						FuriContext->SetDamageInfo(FinalDamageInfo);
					}

					FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(
						BaseDamageEffectClass, 1.0f, ContextHandle);

					if (SpecHandle.IsValid())
					{
						// SetByCaller 주입 (대미지를 깎아야 하므로 음수(-) 처리)
						SpecHandle.Data.Get()->SetSetByCallerMagnitude(
							FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")), -FinalDamageInfo.Amount);

						// 🌟 ToTarget 사용: 대상(TargetChar)에게 피해 적용!
						MyASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(),
						                                       TargetChar->GetAbilitySystemComponent());

						return; // 한 번 타격했으면 스캔 종료
					}
				}
			}
		}
	}
	else
	{
		FGameplayCueParameters HitParams;
		HitParams.Instigator = MyAvatar;
		HitParams.EffectCauser = MyAvatar;
		HitParams.Location = MyAvatar->GetActorLocation();

		MyASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.SFX.Swing")), HitParams);
	}
}

void UGA_TeleportDashAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo,
                                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                                        bool bWasCancelled)
{
	if (VanishCueTag.IsValid() && GetAbilitySystemComponentFromActorInfo())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveGameplayCue(VanishCueTag);
	}

	LockedTarget = nullptr;
	CurrentStrikeCount = 0;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
