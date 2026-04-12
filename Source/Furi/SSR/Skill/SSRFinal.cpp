#include "SSRFinal.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Furi/SSR/SSRPlayer.h"
#include "Furi/FuriPlayerController.h"
#include "Furi/FuriBlueprintFunctionLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Furi/GamePlayAbilitySystem/FuriAbilityTypes.h" // 🌟 커스텀 컨텍스트를 위해 추가

USSRFinal::USSRFinal()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void USSRFinal::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                const FGameplayAbilityActivationInfo ActivationInfo,
                                const FGameplayEventData* TriggerEventData)
{
	// 🌟 [수정] 코스트 및 쿨타임 최우선 검사 (CommitAbility)
	CachedPlayer = Cast<ASSRPlayer>(ActorInfo->AvatarActor.Get());
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || !CachedPlayer)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bFirstHitSuccess = false;
	GrabbedTarget = nullptr;
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

	if (HitEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, HitEventTag);
		WaitEventTask->EventReceived.AddDynamic(this, &USSRFinal::OnHitEventReceived);
		WaitEventTask->ReadyForActivation();
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, TEXT("UltimateTask"), UltimateMontage);
	MontageTask->OnCompleted.AddDynamic(this, &USSRFinal::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &USSRFinal::OnMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &USSRFinal::OnMontageFinished);
	MontageTask->ReadyForActivation();
}

void USSRFinal::OnHitEventReceived(FGameplayEventData Payload)
{
	ProcessPhysicalHit();
}

void USSRFinal::ProcessPhysicalHit()
{
	if (!IsActive() || !CachedPlayer)
	{
		return;
	}

	// [A] 이미 적을 잡은 상태인지 확인 (콤보 구간)
	if (bFirstHitSuccess)
	{
		if (IsValid(GrabbedTarget))
		{
			CurrentHitCount++;

			// 🌟 [핵심] Data Asset에서 대미지 정보를 동적으로 가져옵니다.
			FFuriSkillData SkillData;
			FFuriDamageInfo FinalDamageInfo;

			if (GetCurrentSkillData(SkillData))
			{
				FinalDamageInfo = SkillData.DamageInfo;

				// 막타(5타)일 경우 대미지와 판정을 강하게 보정합니다.
				if (CurrentHitCount >= 5)
				{
					FinalDamageInfo.Amount *= 3.0f; // 대미지 3배 증폭 (예: 10 -> 30)
					FinalDamageInfo.DamageResponse = EFuriDamageResponse::KnockBack;
					FinalDamageInfo.bShouldForceInterrupt = true;
				}
				else
				{
					FinalDamageInfo.DamageResponse = EFuriDamageResponse::HitReaction;
				}
			}
			else
			{
				// 데이터 로드 실패 시 안전장치
				FinalDamageInfo.Amount = (CurrentHitCount >= 5) ? 30.0f : 10.0f;
			}

			// 수정된 함수 호출
			ApplyDamageToTarget(FinalDamageInfo);

			if (HitComboEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitComboEffect,
				                                               GrabbedTarget->GetActorLocation());
			}
		}
		return;
	}

	// [B] 적 탐색 (첫 타격 판정)
	FVector Start = CachedPlayer->GetActorLocation();
	FVector Forward = CachedPlayer->GetActorForwardVector();
	FVector End = Start + (Forward * AttackRange);
	FVector HalfSize = FVector(AttackRange * 0.5f, AttackBoxHalfWidth, AttackBoxHalfHeight);

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(CachedPlayer);

	FHitResult HitResult;
	bool bHit = UKismetSystemLibrary::BoxTraceSingle(
		GetWorld(), Start, End, HalfSize, CachedPlayer->GetActorRotation(),
		UEngineTypes::ConvertToTraceType(ECC_Pawn), false,
		ActorsToIgnore, EDrawDebugTrace::None, HitResult, true,
		FLinearColor::Red, FLinearColor::Green, 1.0f
	);

	if (bHit && HitResult.GetActor())
	{
		ACharacter* TargetChar = Cast<ACharacter>(HitResult.GetActor());

		if (TargetChar)
		{
			bFirstHitSuccess = true;
			GrabbedTarget = TargetChar;

			if (AFuriPlayerController* pc = Cast<AFuriPlayerController>(CurrentActorInfo->PlayerController.Get()))
			{
				pc->SetFuriCinematicMode(true, TargetChar);
			}

			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.7f);

			GrabbedTarget->AttachToComponent(CachedPlayer->GetRootComponent(),
			                                 FAttachmentTransformRules::SnapToTargetNotIncludingScale);

			GrabbedTarget->SetActorRelativeLocation(FVector(120.f, 0.f, 0.f));
			GrabbedTarget->SetActorRelativeRotation(FRotator(0.f, 180.f, 0.f));

			TargetChar->GetCharacterMovement()->DisableMovement();
			TargetChar->GetCharacterMovement()->StopMovementImmediately();

			CachedPlayer->SetCameraZoom(true);

			if (GrabEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAttached(GrabEffect, GrabbedTarget->GetRootComponent(), NAME_None,
				                                             FVector::ZeroVector, FRotator::ZeroRotator,
				                                             EAttachLocation::KeepRelativeOffset, true);
			}

			UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
			if (MyASC)
			{
				MyASC->CurrentMontageJumpToSection(FName("Combo"));
			}
		}
	}
	else
	{
		UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
		if (MyASC)
		{
			MyASC->CurrentMontageJumpToSection(FName("Failed"));
		}
	}
}

// 🌟 [수정] FFuriDamageInfo를 전달받아 완벽한 컨텍스트로 조립합니다.
void USSRFinal::ApplyDamageToTarget(const FFuriDamageInfo& DamageInfo)
{
	UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GrabbedTarget);
	AActor* MyAvatar = GetAvatarActorFromActorInfo();

	// BaseDamageEffectClass 사용
	if (MyASC && TargetASC && BaseDamageEffectClass)
	{
		// 1. 엔진 표준 방식으로 컨텍스트 생성
		FGameplayEffectContextHandle ContextHandle = MyASC->MakeEffectContext();
		ContextHandle.AddInstigator(MyAvatar, MyAvatar);

		// 2. 커스텀 컨텍스트에 DamageInfo 심기
		if (FFuriGameplayEffectContext* FuriContext = FFuriGameplayEffectContext::GetFuriContext(ContextHandle))
		{
			FuriContext->SetDamageInfo(DamageInfo);
		}

		// 3. Spec 생성 및 주입
		FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(BaseDamageEffectClass, GetAbilityLevel(),
		                                                               ContextHandle);

		if (SpecHandle.IsValid())
		{
			FGameplayTag DamageDataTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount"));

			// 주의: 상대방의 체력을 깎아야 하므로 -음수 처리
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageDataTag, -DamageInfo.Amount);

			MyASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}
}

void USSRFinal::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USSRFinal::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                           const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                           bool bWasCancelled)
{
	if (CachedPlayer)
	{
		CachedPlayer->SetCameraZoom(false);
	}

	if (GrabbedTarget)
	{
		GrabbedTarget->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		if (ACharacter* TargetChar = Cast<ACharacter>(GrabbedTarget))
		{
			TargetChar->GetCharacterMovement()->SetDefaultMovementMode();
		}
	}

	if (GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
	}

	if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(CurrentActorInfo->PlayerController.Get()))
	{
		PC->SetFuriCinematicMode(false, nullptr);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
