// Fill out your copyright notice in the Description page of Project Settings.

#include "SSRAttack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/GamePlayAbilitySystem/FuriAbilityTypes.h"
#include "Kismet/GameplayStatics.h"

USSRAttack::USSRAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 🚨 [수정] CDO 크래시 방지를 위해 생성자에서의 태그 하드코딩 삭제!
	// ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Lock"))); 
	// -> 이 부분은 블루프린트 디테일 패널의 'Activation Owned Tags'에서 설정해 주세요.
}

void USSRAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo,
                                 const FGameplayEventData* TriggerEventData)
{
	// 🌟 [수정] 엔진 규격에 맞게 CommitAbility를 가장 먼저 실행하여 코스트/쿨타임 검사
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

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
	UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
	AGasCharacterBase* MyCharacter = Cast<AGasCharacterBase>(MyAvatar);

	if (!MyCharacter || !MyASC)
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

				// 🌟 [수정] Data Asset에서 정보를 동적으로 가져옵니다.
				FFuriSkillData SkillData;
				if (GetCurrentSkillData(SkillData))
				{
					FFuriDamageInfo FinalDamageInfo = SkillData.DamageInfo;

					// 막타(7타) 보정 로직
					if (ActualIndex == 7)
					{
						FinalDamageInfo.Amount *= 1.5f; // 대미지 증폭 (필요에 따라 조절)
						FinalDamageInfo.DamageResponse = EFuriDamageResponse::KnockBack;
						FinalDamageInfo.bCanBeParried = false;
						FinalDamageInfo.bShouldForceInterrupt = true;
					}
					else
					{
						FinalDamageInfo.DamageResponse = EFuriDamageResponse::HitReaction;
						FinalDamageInfo.bCanBeParried = true;
					}

					// 🌟 [수정] ComboDamageMap 대신 BaseDamageEffectClass 사용
					if (BaseDamageEffectClass)
					{
						// 컨텍스트 생성 (new 대신 엔진 표준 방식 MakeEffectContext 사용)
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
							// SetByCaller로 대미지 주입 (음수로 깎기)
							SpecHandle.Data.Get()->SetSetByCallerMagnitude(
								FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")), -FinalDamageInfo.Amount);

							// ApplyGameplayEffectSpecToSelf -> ToTarget으로 변경
							MyASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(),
							                                       Target->GetAbilitySystemComponent());
						}
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
	// 로직 추가 필요 시 구현
}
