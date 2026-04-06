// Fill out your copyright notice in the Description page of Project Settings.

#include "SSRSunFire.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "TimerManager.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/GamePlayAbilitySystem/FuriAbilityTypes.h"
#include "GameFramework/Character.h"

USSRSunFire::USSRSunFire()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 🚨 [수정] CDO 크래시 방지! 생성자에서의 태그 하드코딩 삭제
	// 블루프린트 디테일 패널에서 세팅해 주세요:
	// 1. Ability Tags -> Ability.Action.SunFire
	// 2. Activation Owned Tags -> State.SunFire
}

void USSRSunFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                  const FGameplayEventData* TriggerEventData)
{
	// 🌟 부모 클래스의 스태미나 코스트 및 쿨타임 최우선 검사
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		K2_EndAbility();
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (ASC)
	{
		if (SunFireStartCueTag.IsValid())
		{
			FGameplayCueParameters Params;
			Params.Location = GetAvatarActorFromActorInfo()->GetActorLocation();
			ASC->ExecuteGameplayCue(SunFireStartCueTag, Params);
		}
		if (SunFireLoopCueTag.IsValid())
		{
			ASC->AddGameplayCue(SunFireLoopCueTag);
		}
	}

	// 틱 데미지 타이머 시작
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(TickTimerHandle, this, &USSRSunFire::ApplySunFireDamage, TickInterval,
		                                       true);
	}

	UE_LOG(LogTemp, Log, TEXT("[SunFire] 장판 활성화됨"));
}

void USSRSunFire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                             const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                             bool bWasCancelled)
{
	UE_LOG(LogTemp, Log, TEXT("[SunFire] EndAbility 호출됨 (토글 OFF)"));

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TickTimerHandle);
	}

	if (ActorInfo->AbilitySystemComponent.IsValid())
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

		if (SunFireLoopCueTag.IsValid())
		{
			ASC->RemoveGameplayCue(SunFireLoopCueTag);
		}

		if (SunFireEndCueTag.IsValid())
		{
			FGameplayCueParameters Params;
			Params.Location = GetAvatarActorFromActorInfo()->GetActorLocation();
			ASC->ExecuteGameplayCue(SunFireEndCueTag, Params);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USSRSunFire::ApplySunFireDamage()
{
	AActor* OwnerActor = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();

	if (!OwnerActor || !MyASC || !BaseDamageEffectClass)
	{
		return;
	}

	// 🌟 [핵심] Data Asset에서 장판 데미지 정보를 동적으로 가져옵니다.
	FFuriSkillData SkillData;
	FFuriDamageInfo DamageInfo;

	if (GetCurrentSkillData(SkillData))
	{
		DamageInfo = SkillData.DamageInfo;
	}
	else
	{
		DamageInfo.Amount = 1.0f; // 안전장치 기본값
	}

	// 장판 특성 강제 덮어쓰기 (가드 불가, 리액션 없음)
	DamageInfo.DamageResponse = EFuriDamageResponse::None;
	DamageInfo.bCanBeBlocked = false;
	DamageInfo.bCanBeParried = false;

	TArray<AActor*> OverlappedActors;
	GetActorInRange(OverlappedActors);

	for (AActor* Target : OverlappedActors)
	{
		if (Target == OwnerActor)
		{
			continue;
		}

		AGasCharacterBase* TargetCharacterBase = Cast<AGasCharacterBase>(Target);
		if (TargetCharacterBase)
		{
			// 엔진 표준 방식으로 컨텍스트 조립
			FGameplayEffectContextHandle ContextHandle = MyASC->MakeEffectContext();
			ContextHandle.AddInstigator(OwnerActor, OwnerActor);

			if (FFuriGameplayEffectContext* FuriContext = FFuriGameplayEffectContext::GetFuriContext(ContextHandle))
			{
				FuriContext->SetDamageInfo(DamageInfo);
			}

			FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(
				BaseDamageEffectClass, GetAbilityLevel(), ContextHandle);

			if (SpecHandle.IsValid())
			{
				// 상대방 체력을 깎아야 하므로 음수(-) 처리하여 Data Asset 값 주입
				SpecHandle.Data.Get()->SetSetByCallerMagnitude(
					FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")), -DamageInfo.Amount);

				// 🌟 Target->GetASC()->Apply...ToSelf 대신 MyASC->Apply...ToTarget 사용 (GAS 표준 규약)
				MyASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(),
				                                       TargetCharacterBase->GetAbilitySystemComponent());

				FGameplayCueParameters HitParams;
				HitParams.Location = Target->GetActorLocation();
				MyASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.VFX.Hit")), HitParams);
			}
		}
	}
}

void USSRSunFire::GetActorInRange(TArray<AActor*>& OutActors)
{
	AActor* OwnerActor = GetAvatarActorFromActorInfo();
	if (!OwnerActor || !GetWorld())
	{
		return;
	}

	FVector Center = OwnerActor->GetActorLocation();
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerActor);

	// 중복 검사 제거, Pawn 채널 한 번만 깔끔하게 검사
	bool bHit = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		Center,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(Radius),
		Params
	);

#if !UE_BUILD_SHIPPING
	// 디버그 구체 (에디터에서 확인용)
	DrawDebugSphere(GetWorld(), Center, Radius, 16, FColor::Orange, false, TickInterval);
#endif

	if (bHit)
	{
		for (const FOverlapResult& Result : Overlaps)
		{
			if (AActor* HitActor = Result.GetActor())
			{
				OutActors.AddUnique(HitActor);
			}
		}
	}
}

void USSRSunFire::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                               const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	// 다시 눌렀을 때 토글 OFF
	UE_LOG(LogTemp, Warning, TEXT("[SunFire] 다시 누름 감지 -> 토글 OFF"));
	K2_EndAbility();
}
