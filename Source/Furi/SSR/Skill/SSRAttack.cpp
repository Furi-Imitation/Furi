// Fill out your copyright notice in the Description page of Project Settings.

#include "SSRAttack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/GamePlayAbilitySystem/FuriAbilityTypes.h"
#include "Kismet/GameplayStatics.h"

USSRAttack::USSRAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Lock")));
}

void USSRAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo,
                                 const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentComboIndex = 4;
	bComboWindowOpened = false;
	bNextComboReserved = false;

	UAbilityTask_WaitGameplayEvent* WaitStartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(FName("Combo.Start")));
	WaitStartTask->EventReceived.AddDynamic(this, &USSRAttack::OnComboEventReceived);
	WaitStartTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WaitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(FName("Combo.End")));
	WaitEndTask->EventReceived.AddDynamic(this, &USSRAttack::OnComboEventReceived);
	WaitEndTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* HitCheckTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, HitCheckEventTag);
	HitCheckTask->EventReceived.AddDynamic(this, &USSRAttack::OnComboEventReceived);
	HitCheckTask->ReadyForActivation();

	if (AActor* MyAvatar = GetAvatarActorFromActorInfo())
	{
		RotateTowardsClosestEnemy(MyAvatar, AttackRange);
	}

	PlayComboSection();
}

void USSRAttack::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                              const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	if (bNextComboReserved)
	{
		return;
	}

	if (bComboWindowOpened && CurrentComboIndex < 7)
	{
		bNextComboReserved = true;
		FName CurrentSection = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex);
		FName NextSection = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex + 1);
		MontageSetNextSectionName(CurrentSection, NextSection);
		CurrentComboIndex++;
	}
}

void USSRAttack::OnComboEventReceived(FGameplayEventData Payload)
{
	FGameplayTag EventTag = Payload.EventTag;

	if (EventTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Combo.Start"))))
	{
		bComboWindowOpened = true;
		bNextComboReserved = false;
	}
	else if (EventTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Combo.End"))))
	{
		bComboWindowOpened = false;
	}
	else if (EventTag.MatchesTag(HitCheckEventTag))
	{
		PerformHitCheck();
	}
}

void USSRAttack::PlayComboSection()
{
	if (!ComboMontage)
	{
		return;
	}

	FName SectionName = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex);
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, TEXT("AttackTask"), ComboMontage, 1.0f, SectionName);
	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &USSRAttack::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &USSRAttack::OnMontageCompleted);
		MontageTask->ReadyForActivation();
	}
}

void USSRAttack::PerformHitCheck()
{
	AActor* MyAvatar = GetAvatarActorFromActorInfo();
	AGasCharacterBase* MyCharacter = Cast<AGasCharacterBase>(MyAvatar);
	if (!MyCharacter)
	{
		return;
	}

	int32 ActualIndex = CurrentComboIndex;
	UAnimInstance* AnimInstance = MyCharacter->GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		FName CurrentSection = AnimInstance->Montage_GetCurrentSection(ComboMontage);
		FString SectionStr = CurrentSection.ToString();
		if (SectionStr.Contains(TEXT("4")))
		{
			ActualIndex = 4;
		}
		else if (SectionStr.Contains(TEXT("5")))
		{
			ActualIndex = 5;
		}
		else if (SectionStr.Contains(TEXT("6")))
		{
			ActualIndex = 6;
		}
		else if (SectionStr.Contains(TEXT("7")))
		{
			ActualIndex = 7;
		}
	}

	FVector Forward = MyAvatar->GetActorForwardVector();
	FVector BoxHalfExtent = FVector(150.f, 100.f, 100.f);
	FVector BoxCenter = MyAvatar->GetActorLocation() + (Forward * 150.f);

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(MyAvatar);

	bool bHit = GetWorld()->OverlapMultiByChannel(Overlaps, BoxCenter, MyAvatar->GetActorRotation().Quaternion(),
	                                              ECC_Pawn, FCollisionShape::MakeBox(BoxHalfExtent), Params);

	if (bHit)
	{
		TArray<AActor*> HitActors;
		for (auto& Result : Overlaps)
		{
			AActor* OverlappedActor = Result.GetActor();
			if (!OverlappedActor || HitActors.Contains(OverlappedActor))
			{
				continue;
			}

			if (AGasCharacterBase* Target = Cast<AGasCharacterBase>(OverlappedActor))
			{
				HitActors.Add(OverlappedActor);

				FFuriDamageInfo DamageInfo;
				DamageInfo.Amount = (ActualIndex == 7) ? -30.f : -10.f;
				DamageInfo.DamageResponse = (ActualIndex == 7)
					                            ? EFuriDamageResponse::KnockBack
					                            : EFuriDamageResponse::HitReaction;
				DamageInfo.bCanBeParried = (ActualIndex < 7);
				DamageInfo.bCanBeBlocked = true;

				TSubclassOf<UGameplayEffect> DamageGE = ComboDamageMap.Contains(ActualIndex)
					                                        ? ComboDamageMap[ActualIndex]
					                                        : nullptr;
				if (DamageGE)
				{
					FGameplayEffectContextHandle ContextHandle = FGameplayEffectContextHandle(
						new FFuriGameplayEffectContext());
					ContextHandle.AddInstigator(MyAvatar, MyAvatar);

					FFuriGameplayEffectContext* FuriContext = FFuriGameplayEffectContext::GetFuriContext(ContextHandle);
					if (FuriContext)
					{
						FuriContext->SetDamageInfo(DamageInfo);
					}

					FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageGE);
					if (Spec.IsValid())
					{
						Spec.Data.Get()->SetContext(ContextHandle);
						Spec.Data.Get()->SetSetByCallerMagnitude(
							FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")), DamageInfo.Amount);
						Target->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
					}
				}
			}
		}
	}

#if !UE_BUILD_SHIPPING
	DrawDebugBox(GetWorld(), BoxCenter, BoxHalfExtent, MyAvatar->GetActorRotation().Quaternion(), FColor::Red, false,
	             2.0f, 0, 2.0f);
#endif
}

void USSRAttack::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USSRAttack::RotateTowardsClosestEnemy(AActor* MyAvatar, float SearchRadius)
{
}
