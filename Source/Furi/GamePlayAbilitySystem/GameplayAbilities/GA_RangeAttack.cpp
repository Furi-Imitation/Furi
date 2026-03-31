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

	// 실행 중 무적을 위해 OwnedTags에 추가
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Invincible")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Lock")));

	HitEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Montage.Hit"));

	StartCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.VFX.RangeAttack.Start"));
	HitCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.VFX.RangeAttack"));

	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Lock")));
}

void UGA_RangeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();

	// 스킬 시작 즉시 Start Cue 실행
	if (MyASC && StartCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = GetAvatarActorFromActorInfo()->GetActorLocation();
		Params.Instigator = GetAvatarActorFromActorInfo();
		Params.NormalizedMagnitude = 3.f; // 나이아가라 속도 조절용 (User.PlayRate)
		MyASC->ExecuteGameplayCue(StartCueTag, Params);
		UE_LOG(LogTemp, Log, TEXT("[RangeAttack] Start Cue 실행"));
	}

	// 1. 🌟 노티파이(GameplayEvent) 대기 태스크 생성
	UAbilityTask_WaitGameplayEvent* WaitEventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitEventTag);
	if (WaitEventTask)
	{
		WaitEventTask->EventReceived.AddDynamic(this, &UGA_RangeAttack::OnHitEventReceived);
		WaitEventTask->ReadyForActivation();
	}

	//몽타주 재생 태스크 생성
	UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, TEXT("RangeAttack"), RangeAttackMontage);
	if (PlayMontageTask)
	{
		// 종료/취소/방해 시 모두 EndAbility를 타도록 설정
		PlayMontageTask->OnCompleted.AddDynamic(this, &UGA_RangeAttack::OnMontageFinished);
		PlayMontageTask->OnInterrupted.AddDynamic(this, &UGA_RangeAttack::OnMontageFinished);
		PlayMontageTask->OnCancelled.AddDynamic(this, &UGA_RangeAttack::OnMontageFinished);
		PlayMontageTask->ReadyForActivation();
	}
}

// FFuriDamageInfo를 사용하는 데미지 로직
void UGA_RangeAttack::OnHitEventReceived(FGameplayEventData Payload)
{
	AActor* MyAvatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
	if (!MyAvatar || !MyASC)
	{
		return;
	}


	FVector Origin = MyAvatar->GetActorLocation();
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(MyAvatar);

	if (HitCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = Origin;
		CueParams.RawMagnitude = AttackRadius; // 나이아가라 크기 조절용 (User.Radius)
		CueParams.NormalizedMagnitude = 1.5f; // 나이아가라 속도 조절용 (User.PlayRate)
		// Params.Instigator 등을 통해 추가 데이터 전달 가능

		MyASC->ExecuteGameplayCue(HitCueTag, CueParams);
		UE_LOG(LogTemp, Log, TEXT("[RangeAttack] Hit Cue 실행 (타격 시점)"));
	}

	if (GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RangeAttack] 10M 디버그 구체 생성!"));

		DrawDebugSphere(
			GetWorld(),
			Origin,
			AttackRadius,
			32,
			FColor::Red,
			false,
			1.0f
		);
	}
	// 10미터 범위 스캔
	bool bHit = GetWorld()->OverlapMultiByChannel(OverlapResults, Origin, FQuat::Identity, ECC_Pawn,
	                                              FCollisionShape::MakeSphere(AttackRadius), Params);

	if (bHit)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			//사용자님의 캐릭터 베이스로 캐스팅
			AGasCharacterBase* TargetCharacter = Cast<AGasCharacterBase>(Result.GetActor());

			if (TargetCharacter && DamageEffectClass)
			{
				// 1. 사용자님 스타일의 커스텀 정보 조립
				FFuriDamageInfo DamageInfo;
				DamageInfo.Amount = -50.f; // 광역기 데미지
				DamageInfo.DamageType = EFuriDamageType::Melee; // 혹은 Range로 변경 가능
				DamageInfo.DamageResponse = EFuriDamageResponse::KnockBack; // 10미터 밖으로 날려버리기

				// 광역 장판 공격이므로 패링 불가 설정
				DamageInfo.bCanBeParried = false;
				DamageInfo.bCanBeBlocked = true;
				DamageInfo.bShouldForceInterrupt = true;

				// 2. GE Spec 작성 (피를 깎는 용도)
				// [수정] 기본 MakeEffectContext 대신 커스텀 컨텍스트를 직접 생성합니다.
				FGameplayEffectContextHandle ContextHandle = FGameplayEffectContextHandle(
					new FFuriGameplayEffectContext());
				ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());

				FFuriGameplayEffectContext* FuriContext = FFuriGameplayEffectContext::GetFuriContext(ContextHandle);
				if (FuriContext)
				{
					FuriContext->SetDamageInfo(DamageInfo);
				}

				FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);

				if (SpecHandle.IsValid())
				{
					// GE 수치 주입
					SpecHandle.Data.Get()->SetSetByCallerMagnitude(
						FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")), DamageInfo.Amount);

					// 타겟에게 GE 적용 (AttributeSet에서 리액션 처리)
					TargetCharacter->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

					UE_LOG(LogTemp, Log, TEXT("[RangeAttack] %s에게 10M 광역 피해와 넉백 적용!"), *TargetCharacter->GetName());
				}

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

// 🌟 EndAbility 오버라이드
void UGA_RangeAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                                 bool bWasCancelled)
{
	// 여기서 추가적인 정리 로직을 넣을 수 있습니다.
	// (예: 강제 종료 시 생성 중이던 나이아가라 제거 등)

	UE_LOG(LogTemp, Log, TEXT("RangeAttack 스킬 종료 및 상태 정리 완료"));

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_RangeAttack::OnMontageFinished()
{
	// 몽타주가 끝나면 EndAbility 호출
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
