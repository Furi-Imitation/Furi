#include "GA_Attack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/GamePlayAbilitySystem/FuriAbilityTypes.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UGA_Attack::UGA_Attack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo,
                                 const FGameplayEventData* TriggerEventData)
{
	// 🌟 Data Asset 코스트/쿨타임 적용 (UFuriGameplayAbilityBase 기능 발동)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CurrentComboIndex = 1;
	bComboWindowOpened = false;
	bNextComboReserved = false;

	// 태스크 생성
	UAbilityTask_WaitGameplayEvent* WaitStartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(FName("Combo.Start")));
	WaitStartTask->EventReceived.AddDynamic(this, &UGA_Attack::OnComboEventReceived);
	WaitStartTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WaitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(FName("Combo.End")));
	WaitEndTask->EventReceived.AddDynamic(this, &UGA_Attack::OnComboEventReceived);
	WaitEndTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* HitCheckTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, HitCheckEventTag);
	HitCheckTask->EventReceived.AddDynamic(this, &UGA_Attack::OnComboEventReceived);
	HitCheckTask->ReadyForActivation();

	if (AActor* MyAvatar = GetAvatarActorFromActorInfo())
	{
		RotateTowardsClosestEnemy(MyAvatar, AttackRange);
	}

	PlayComboSection();
}

void UGA_Attack::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                              const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	if (bComboWindowOpened && !bNextComboReserved && CurrentComboIndex < 3)
	{
		bNextComboReserved = true;

		if (AActor* MyAvatar = GetAvatarActorFromActorInfo())
		{
			RotateTowardsClosestEnemy(MyAvatar, AttackRange);
		}

		FName CurrentSection = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex);
		FName NextSection = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex + 1);

		MontageSetNextSectionName(CurrentSection, NextSection);
		CurrentComboIndex++;
	}
}

void UGA_Attack::OnComboEventReceived(FGameplayEventData Payload)
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

void UGA_Attack::PlayComboSection()
{
	if (!ComboMontage)
	{
		return;
	}

	FName SectionName = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex);

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, TEXT("AttackTask"), ComboMontage, 1.0f, SectionName);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_Attack::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Attack::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Attack::OnMontageCompleted);
	MontageTask->ReadyForActivation();
}

void UGA_Attack::PerformHitCheck()
{
	AActor* MyAvatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
	ACharacter* MyCharacter = Cast<ACharacter>(MyAvatar);

	if (!MyAvatar || !MyASC || !MyCharacter)
	{
		return;
	}

	// 현재 콤보 인덱스 보정 로직 (유지)
	int32 ActualComboIndex = CurrentComboIndex;
	if (UAnimInstance* AnimInstance = MyCharacter->GetMesh()->GetAnimInstance())
	{
		FName CurrentSectionName = AnimInstance->Montage_GetCurrentSection(ComboMontage);
		FString SectionString = CurrentSectionName.ToString();

		if (SectionString.Contains(TEXT("1")))
		{
			ActualComboIndex = 1;
		}
		else if (SectionString.Contains(TEXT("2")))
		{
			ActualComboIndex = 2;
		}
		else if (SectionString.Contains(TEXT("3")))
		{
			ActualComboIndex = 3;
		}
	}

	// 충돌 판정 계산
	FVector AvatarLocation = MyAvatar->GetActorLocation();
	FRotator AvatarRotation = MyAvatar->GetActorRotation();
	FVector Forward = MyAvatar->GetActorForwardVector();
	FVector BoxHalfExtent = FVector(AttackRange / 2.0f, AttackBoxHalfWidth, AttackBoxHalfHeight);
	FVector BoxCenter = AvatarLocation + (Forward * (AttackRange / 2.0f));

	if (UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(MyAvatar->GetRootComponent()))
	{
		BoxCenter.Z = AvatarLocation.Z - Capsule->GetScaledCapsuleHalfHeight() + BoxHalfExtent.Z;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(MyAvatar);
	FCollisionShape BoxShape = FCollisionShape::MakeBox(BoxHalfExtent);
	bool bOverlapHit = GetWorld()->OverlapMultiByChannel(OverlapResults, BoxCenter, AvatarRotation.Quaternion(),
	                                                     ECC_Pawn, BoxShape, QueryParams);

	AActor* ClosestTarget = nullptr;
	float ClosestDistanceSq = FMath::Square(AttackRange + 100.0f);

	if (bOverlapHit)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			if (AActor* OverlappedActor = Result.GetActor())
			{
				if (OverlappedActor->IsA<APawn>())
				{
					float DistanceSq = FVector::DistSquared(AvatarLocation, OverlappedActor->GetActorLocation());
					if (DistanceSq < ClosestDistanceSq)
					{
						ClosestDistanceSq = DistanceSq;
						ClosestTarget = OverlappedActor;
					}
				}
			}
		}
	}

	if (ClosestTarget)
	{
		AGasCharacterBase* TargetCharacter = Cast<AGasCharacterBase>(ClosestTarget);
		if (TargetCharacter && MyASC)
		{
			// Data Asset에서 스킬 데이터를 가져옵니다.
			FFuriSkillData SkillData;
			if (GetCurrentSkillData(SkillData))
			{
				// 원본 데이터를 복사합니다.
				FFuriDamageInfo FinalDamageInfo = SkillData.DamageInfo;

				// 막타(3타)일 경우 대미지와 판정을 강하게 보정합니다.
				if (ActualComboIndex == 3)
				{
					FinalDamageInfo.Amount *= 1.5f; // 대미지 1.5배 (예: 20 -> 30)
					FinalDamageInfo.DamageResponse = EFuriDamageResponse::KnockBack;
					FinalDamageInfo.bCanBeParried = false;
					FinalDamageInfo.bShouldForceInterrupt = true;
				}

				// 타격 이펙트 및 카메라 셰이크 재생
				FGameplayCueParameters HitParams;
				HitParams.Instigator = MyAvatar;
				HitParams.EffectCauser = MyAvatar;
				HitParams.Location = ClosestTarget->GetActorLocation();

				MyASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.VFX.Hit")), HitParams);
				FGameplayTag HitSFXTag = (ActualComboIndex == 3)
					                         ? FGameplayTag::RequestGameplayTag(
						                         FName("GameplayCue.P1.SFX.Attack.Large"))
					                         : FGameplayTag::RequestGameplayTag(
						                         FName("GameplayCue.P1.SFX.Attack.Small"));
				MyASC->ExecuteGameplayCue(HitSFXTag, HitParams);
				MyASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.CameraShake.Hit")),
				                          HitParams);

				// 대미지 적용 (BaseDamageEffectClass 사용)
				if (BaseDamageEffectClass)
				{
					// 컨텍스트 생성 (알맞은 캐스팅 방식 적용)
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
						// 🌟 Data Asset의 값을 SetByCaller로 주입! (체력을 깎아야 하므로 -음수 처리)
						SpecHandle.Data.Get()->SetSetByCallerMagnitude(
							FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")), -FinalDamageInfo.Amount);

						// 🌟 주의: ApplyGameplayEffectSpecToSelf가 아니라 ToTarget이어야 합니다!
						MyASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(),
						                                       TargetCharacter->GetAbilitySystemComponent());
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

void UGA_Attack::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Attack::RotateTowardsClosestEnemy(AActor* MyAvatar, float SearchRadius)
{
	if (!MyAvatar || !GetWorld())
	{
		return;
	}
	FVector AvatarLocation = MyAvatar->GetActorLocation();
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(MyAvatar);
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(SearchRadius + 3000);

	bool bOverlapHit = GetWorld()->OverlapMultiByChannel(OverlapResults, AvatarLocation, FQuat::Identity, ECC_Pawn,
	                                                     SphereShape, Params);

	AActor* ClosestEnemy = nullptr;
	float ClosestDistanceSq = FMath::Square(SearchRadius + 3000.0f);

	if (bOverlapHit)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			if (AActor* Actor = Result.GetActor(); Actor && Actor->IsA<APawn>())
			{
				float DistSq = FVector::DistSquared(AvatarLocation, Actor->GetActorLocation());
				if (DistSq < ClosestDistanceSq)
				{
					ClosestDistanceSq = DistSq;
					ClosestEnemy = Actor;
				}
			}
		}
	}

	if (ClosestEnemy)
	{
		FVector TargetLocation = ClosestEnemy->GetActorLocation();
		TargetLocation.Z = AvatarLocation.Z;
		FRotator LookAtRotation = FRotationMatrix::MakeFromX(TargetLocation - AvatarLocation).Rotator();
		MyAvatar->SetActorRotation(LookAtRotation);
	}
}
