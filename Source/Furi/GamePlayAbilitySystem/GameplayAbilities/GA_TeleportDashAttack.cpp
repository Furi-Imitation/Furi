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
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Lock")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Lock")));
}

void UGA_TeleportDashAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo,
                                             const FGameplayAbilityActivationInfo ActivationInfo,
                                             const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	CurrentStrikeCount = 0;
	AActor* MyAvatar = GetAvatarActorFromActorInfo();
	TArray<AActor*> IgnoredActors;

	LockedTarget = UFuriBlueprintFunctionLibrary::FindClosestTarget(MyAvatar, 2000.f, IgnoredActors);

	// 타겟이 유효한지 안전하게 검사
	if (!IsValid(LockedTarget))
	{
		UE_LOG(LogTemp, Warning, TEXT("텔레포트 할 타겟이 없습니다!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 타격 이벤트 리스너
	UAbilityTask_WaitGameplayEvent* WaitHitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, HitEventTag);
	if (WaitHitEventTask)
	{
		WaitHitEventTask->EventReceived.AddDynamic(this, &UGA_TeleportDashAttack::OnHitCheckEventReceived);
		WaitHitEventTask->ReadyForActivation();
	}

	// 바로 1타 준비(숨기 -> 대기) 모드로 진입
	PrepareNextStrike();
}

// 숨고 대기하기 (매 타격마다 반복됨)
void UGA_TeleportDashAttack::PrepareNextStrike()
{
	ACharacter* MyChar = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	// 3타를 다 쳤거나, 때리는 도중 타겟이 죽어버렸으면 어빌리티를 종료합니다.
	if (!MyChar || !IsValid(LockedTarget) || CurrentStrikeCount >= MaxStrikeCount)
	{
		bool bWasCancelled = (CurrentStrikeCount < MaxStrikeCount);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
		return;
	}

	// [연출] 큐를 켜서 캐릭터를 투명하게 만듭니다.
	if (VanishCueTag.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->AddGameplayCue(VanishCueTag);
	}
	// [Gameplay Cue] 사운드나 파티클 같은 '시각적 효과'를 실행합니다.
	if (TeleportCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = MyChar->GetActorLocation();
		Params.Instigator = MyChar;
		GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(TeleportCueTag, Params);
	}

	// [대기] 설정된 StrikeDelay(0.5초)만큼 기다리는 태스크 실행
	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, StrikeDelay);
	if (DelayTask)
	{
		DelayTask->OnFinish.AddDynamic(this, &UGA_TeleportDashAttack::OnStrikeDelayFinished);
		DelayTask->ReadyForActivation();
	}
}

// 나타나서 타격하기 (0.5초 대기가 끝나면 실행)
void UGA_TeleportDashAttack::OnStrikeDelayFinished()
{
	ACharacter* MyChar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* MyASC = GetAvatarActorFromActorInfo()
		                                 ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(
			                                 GetAvatarActorFromActorInfo())
		                                 : nullptr;

	if (!MyChar)
	{
		return;
	}

	// [연출] 타격 직전, 투명화를 해제하여 모습을 드러냅니다!
	if (VanishCueTag.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveGameplayCue(VanishCueTag);
	}
	// 휘두르기 연출 (VFX Trail) - ActualComboIndex 사용
	FGameplayCueParameters SwingParams;
	SwingParams.Instigator = MyChar;
	SwingParams.TargetAttachComponent = MyChar->GetMesh();

	FGameplayTag AttackVFXTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.VFX.Attack.Large"));

	MyASC->ExecuteGameplayCue(AttackVFXTag, SwingParams);

	// [위치 이동] 1, 2, 3타 위치 계산 및 텔레포트 실행
	FVector TeleportLoc;
	FRotator TeleportRot;
	UFuriBlueprintFunctionLibrary::CalculateTeleportTransform(LockedTarget, CurrentStrikeCount + 1, 350.f, TeleportLoc,
	                                                          TeleportRot);

	MyChar->SetActorLocationAndRotation(TeleportLoc, TeleportRot, false, nullptr, ETeleportType::TeleportPhysics);

	// 3. [Gameplay Cue] 사운드나 파티클 같은 '시각적 효과'를 실행합니다.
	if (TeleportCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = MyChar->GetActorLocation();
		Params.Instigator = MyChar;
		GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(TeleportCueTag, Params);
	}

	// [공격] 몽타주 재생
	if (StrikeMontages.IsValidIndex(CurrentStrikeCount) && StrikeMontages[CurrentStrikeCount])
	{
		UAnimMontage* MontageToPlay = StrikeMontages[CurrentStrikeCount];

		UAbilityTask_PlayMontageAndWait* PlayMontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, TEXT("StrikeMontage"), MontageToPlay);

		if (PlayMontageTask)
		{
			// 몽타주가 끝나면 다시 숨고 대기하는 PrepareNextStrike로 순환
			PlayMontageTask->OnCompleted.AddDynamic(this, &UGA_TeleportDashAttack::OnStrikeMontageFinished);
			PlayMontageTask->OnInterrupted.AddDynamic(this, &UGA_TeleportDashAttack::OnStrikeInterrupted);
			PlayMontageTask->OnCancelled.AddDynamic(this, &UGA_TeleportDashAttack::OnStrikeInterrupted);

			PlayMontageTask->ReadyForActivation();
		}
	}


	// 타수 증가
	CurrentStrikeCount++;
}

// 한 타수가 끝났을 때 다시 루프로 연결
void UGA_TeleportDashAttack::OnStrikeMontageFinished()
{
	// 애니메이션이 끝나면 다시 0.5초 대기(Prepare) 모드로 들어갑니다.
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

	// 박스 판정 위치 및 크기 설정
	// 캐릭터 위치에서 전방(Forward)으로 AttackRange의 절반만큼 떨어진 곳이 박스의 중심입니다.
	FVector Forward = MyAvatar->GetActorForwardVector();
	FVector BoxCenter = MyAvatar->GetActorLocation() + (Forward * (AttackRange * 0.5f));

	// 박스의 절반 크기 (HalfExtent): X는 사거리의 절반, Y는 너비 절반, Z는 높이 절반
	FVector BoxHalfExtent = FVector(AttackRange * 0.5f, AttackBoxHalfWidth, AttackBoxHalfHeight);

	// 캐릭터의 회전값을 적용하여 박스도 캐릭터 방향으로 회전시킵니다.
	FQuat BoxRotation = MyAvatar->GetActorQuat();

	// 물리 스캔 실행 (Box Overlap)
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(MyAvatar);

	bool bHit = GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		BoxCenter,
		BoxRotation,
		ECC_Pawn, // 캐릭터 채널
		FCollisionShape::MakeBox(BoxHalfExtent),
		QueryParams
	);

	// 디버그용 박스 그리기 (개발 단계에서 판정 범위를 눈으로 확인하고 싶을 때 사용)
	DrawDebugBox(GetWorld(), BoxCenter, BoxHalfExtent, BoxRotation, FColor::Red, false, 1.0f);

	if (bHit)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			// 내가 락온한 적이 박스 안에 들어왔는지 확인
			if (Result.GetActor() == LockedTarget)
			{
				AGasCharacterBase* TargetChar = Cast<AGasCharacterBase>(LockedTarget);
				if (TargetChar && DamageGEClass)
				{
					FGameplayCueParameters HitParams;
					HitParams.Instigator = MyAvatar;
					HitParams.EffectCauser = MyAvatar;
					HitParams.Location = TargetChar->GetActorLocation();

					MyASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.CameraShake.Hit")),
					                          HitParams);

					// --- 대미지 정보 설정 ---
					FFuriDamageInfo DamageInfo;
					DamageInfo.Amount = (CurrentStrikeCount == MaxStrikeCount) ? -30.f : -15.f;
					DamageInfo.DamageType = EFuriDamageType::Melee;
					DamageInfo.DamageResponse = (CurrentStrikeCount == MaxStrikeCount)
						                            ? EFuriDamageResponse::KnockBack
						                            : EFuriDamageResponse::Stagger;
					DamageInfo.bCanBeParried = (CurrentStrikeCount < MaxStrikeCount);
					DamageInfo.bCanBeBlocked = true;

					// --- GE Spec 생성 및 대미지 주입 ---
					// 기본 MakeEffectContext 대신 커스텀 컨텍스트를 직접 생성합니다.
					FGameplayEffectContextHandle ContextHandle = FGameplayEffectContextHandle(
						new FFuriGameplayEffectContext());
					ContextHandle.AddInstigator(MyAvatar, MyAvatar);

					FFuriGameplayEffectContext* FuriContext = FFuriGameplayEffectContext::GetFuriContext(ContextHandle);
					if (FuriContext)
					{
						FuriContext->SetDamageInfo(DamageInfo);
					}

					FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
						DamageGEClass, 1.0f, ContextHandle);

					if (SpecHandle.IsValid())
					{
						// Data.Damage.Amount 태그를 통해 실제 수치 전달
						SpecHandle.Data.Get()->SetSetByCallerMagnitude(
							FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")),
							DamageInfo.Amount
						);

						// 🌟 최종 대미지 적용 (AttributeSet에서 리액션 처리)
						TargetChar->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

						// 타격 성공 시 루프 중단 (한 번만 맞게)
						return;
					}
				}
			}
		}
	}
}

void UGA_TeleportDashAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo,
                                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                                        bool bWasCancelled)
{
	// [안전장치] 만약 0.5초 대기 중 스킬이 캔슬되었다면 투명화 강제 해제
	if (VanishCueTag.IsValid() && GetAbilitySystemComponentFromActorInfo())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveGameplayCue(VanishCueTag);
	}

	LockedTarget = nullptr;
	CurrentStrikeCount = 0;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
