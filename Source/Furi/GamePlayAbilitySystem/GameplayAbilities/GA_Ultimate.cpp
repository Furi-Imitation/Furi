#include "GA_Ultimate.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Furi/FuriPlayerController.h"
#include "Furi/FuriBlueprintFunctionLibrary.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/GamePlayAbilitySystem/FuriAbilityTypes.h"
#include "Furi/utils/FuriTypes.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

UGA_Ultimate::UGA_Ultimate()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Ultimate::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayAbilityActivationInfo ActivationInfo,
                                   const FGameplayEventData* TriggerEventData)
{
	// 🌟 [핵심] 코스트 및 쿨타임 소모 확인! (빠져있던 부분)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bFirstHitSuccess = false;
	CurrentHitCount = 0;

	// 🌟 [추가] 시전 시 가장 가까운 적을 바라보도록 회전
	if (AActor* MyAvatar = GetAvatarActorFromActorInfo())
	{
		// 3000.f 반경 내에서 가장 가까운 적을 찾습니다.
		AActor* ClosestTarget = UFuriBlueprintFunctionLibrary::FindClosestTarget(MyAvatar, 3000.f, TArray<AActor*>());
		if (ClosestTarget)
		{
			FVector TargetLocation = ClosestTarget->GetActorLocation();
			FVector MyLocation = MyAvatar->GetActorLocation();
			TargetLocation.Z = MyLocation.Z; // 높이 차이 무시

			FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(MyLocation, TargetLocation);
			MyAvatar->SetActorRotation(LookAtRot);
		}
	}

	// 1. 타격 이벤트 대기
	if (HitEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* WaitHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, HitEventTag);
		if (WaitHitTask)
		{
			WaitHitTask->EventReceived.AddDynamic(this, &UGA_Ultimate::OnHitEventReceived);
			WaitHitTask->ReadyForActivation();
		}
	}

	// 2. 몽타주 실행
	if (UltimateMontage)
	{
		UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("UltTask"), UltimateMontage, 1.0f, TEXT("Dash"));
		if (PlayTask)
		{
			PlayTask->OnCompleted.AddDynamic(this, &UGA_Ultimate::OnMontageFinished);
			PlayTask->OnInterrupted.AddDynamic(this, &UGA_Ultimate::OnMontageFinished);
			PlayTask->OnCancelled.AddDynamic(this, &UGA_Ultimate::OnMontageFinished);
			PlayTask->ReadyForActivation();
		}
	}
}

void UGA_Ultimate::OnHitEventReceived(FGameplayEventData Payload)
{
	ProcessPhysicalHit();
}

void UGA_Ultimate::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Ultimate::ProcessPhysicalHit()
{
	AActor* MyAvatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
	if (!MyAvatar || !MyASC)
	{
		return;
	}

	// --- 물리 판정 로직 ---
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

	// --- 🌟 타격 성공 시 처리 ---
	if (ClosestTarget)
	{
		CurrentHitCount++;

		// [1타 적중 시] 대상을 포착하고 시네마틱 모드 진입
		if (CurrentHitCount == 1 && !bFirstHitSuccess)
		{
			bFirstHitSuccess = true;

			if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(CurrentActorInfo->PlayerController.Get()))
			{
				PC->SetFuriCinematicMode(true, ClosestTarget);
			}

			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.3f);

			GrabbedTarget = ClosestTarget;
			GrabbedTarget->AttachToComponent(MyAvatar->GetRootComponent(),
			                                 FAttachmentTransformRules::KeepWorldTransform);
			GrabbedTarget->SetActorRelativeLocation(FVector(100.0f, 0.0f, 0.0f));

			MontageJumpToSection(TEXT("Cinematic_Combo"));
		}

		// --- Gameplay Cue ---
		FGameplayCueParameters HitParams;
		HitParams.Instigator = MyAvatar;
		HitParams.Location = ClosestTarget->GetActorLocation();

		if (HitVFXCueTag.IsValid())
		{
			MyASC->ExecuteGameplayCue(HitVFXCueTag, HitParams);
		}
		if (HitCameraShakeCueTag.IsValid())
		{
			MyASC->ExecuteGameplayCue(HitCameraShakeCueTag, HitParams);
		}

		FGameplayTag HitSFXTag = (CurrentHitCount == 4)
			                         ? FGameplayTag::RequestGameplayTag(
				                         FName("GameplayCue.P1.SFX.Ultimate"))
			                         : (CurrentHitCount == 1)
			                         ? FGameplayTag::RequestGameplayTag(
				                         FName("GameplayCue.P1.SFX.Attack.Small"))
			                         : FGameplayTag::RequestGameplayTag(
				                         FName("GameplayCue.P1.SFX.Attack.Large"));
		MyASC->ExecuteGameplayCue(HitSFXTag, HitParams);

		bool bIsFinisher = (CurrentHitCount >= 4);

		// --- 🌟 대미지 적용 (Data Asset 연동) ---
		AGasCharacterBase* TargetCharacter = Cast<AGasCharacterBase>(ClosestTarget);
		if (TargetCharacter && BaseDamageEffectClass)
		{
			FFuriSkillData SkillData;
			bool bHasData = GetCurrentSkillData(SkillData);

			FFuriDamageInfo DamageInfo;
			if (bHasData)
			{
				DamageInfo = SkillData.DamageInfo;
			}
			else
			{
				DamageInfo.Amount = 10.f; // 안전장치 기본값
			}

			// 연타 중에는 대미지 0, 막타에만 Data Asset의 전체 대미지 부여!
			DamageInfo.Amount = bIsFinisher ? DamageInfo.Amount * 10 : DamageInfo.Amount;
			DamageInfo.DamageType = EFuriDamageType::Melee;
			DamageInfo.DamageResponse = bIsFinisher ? EFuriDamageResponse::KnockBack : EFuriDamageResponse::HitReaction;
			DamageInfo.bCanBeParried = false;
			DamageInfo.bCanBeBlocked = false;
			DamageInfo.bShouldForceInterrupt = bIsFinisher;

			FGameplayEffectContextHandle ContextHandle = FGameplayEffectContextHandle(new FFuriGameplayEffectContext());
			ContextHandle.AddInstigator(MyAvatar, MyAvatar);

			if (FFuriGameplayEffectContext* FuriContext = FFuriGameplayEffectContext::GetFuriContext(ContextHandle))
			{
				FuriContext->SetDamageInfo(DamageInfo);
			}

			FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(BaseDamageEffectClass, 1.0f, ContextHandle);

			if (SpecHandle.IsValid())
			{
				SpecHandle.Data.Get()->SetSetByCallerMagnitude(
					FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")), -DamageInfo.Amount);

				MyASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(),
				                                       TargetCharacter->GetAbilitySystemComponent());
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

void UGA_Ultimate::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                              const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                              bool bWasCancelled)
{
	if (GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
	}

	if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(CurrentActorInfo->PlayerController.Get()))
	{
		PC->SetFuriCinematicMode(false, nullptr);
	}

	if (GrabbedTarget)
	{
		GrabbedTarget->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		GrabbedTarget = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
