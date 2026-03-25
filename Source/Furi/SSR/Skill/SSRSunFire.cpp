// Fill out your copyright notice in the Description page of Project Settings.


#include "SSRSunFire.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "TimerManager.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/utils/FuriTypes.h"
#include "GameFramework/Character.h"

USSRSunFire::USSRSunFire()
{
	// :star2: 인스턴싱 정책: 액터당 하나만 존재하게 설정
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 어빌리티 식별 태그
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Action.SunFire")));
	
	// :star2: 실행 중일 때 캐릭터에게 부여될 태그 (이 태그로 실행 여부를 판단합니다)
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.SunFire")));

	// 기본 태그 설정 (에디터에서도 설정 가능)
	SunFireStartCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.SSR.SunFire.Start"));
	SunFireLoopCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.SSR.SunFire.Loop"));
	SunFireEndCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.SSR.SunFire.End"));
}

void USSRSunFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC) return;

	// =========================================================
	// :star2: 토글 로직의 핵심
	// =========================================================
	// 현재 캐릭터에게 'State.SunFire' 태그가 이미 있다면? -> 끄고 싶어서 다시 누른 것!
	// 단, 자기 자신(현재 막 실행된 인스턴스)의 태그는 제외하고 검사해야 하므로
	// 실행 중인 다른 '동일 태그' 어빌리티를 찾아서 취소시킵니다.
	TArray<FGameplayAbilitySpec*> ActiveAbilities;
	
	// 현재 캐릭터가 가진 모든 어빌리티 스펙을 가져옵니다.
	TArray<FGameplayAbilitySpec>& AbilitySpecs = ASC->GetActivatableAbilities();
	
	for (FGameplayAbilitySpec& Spec : AbilitySpecs)
	{
		// 1. 나 자신(지금 막 생성된 인스턴스)은 건너뜁니다.
		if (Spec.Handle == Handle) continue;
		// 2. 실행 중이며, 동일한 AbilityTag를 가진 녀석을 찾습니다.
		if (Spec.IsActive() && Spec.Ability->AbilityTags.HasAll(AbilityTags))
		{
			UE_LOG(LogTemp, Warning, TEXT("[SunFire] 기존 실행 중인 어빌리티 감지 -> 토글 OFF"));
			// 기존 어빌리티 취소
			ASC->CancelAbilityHandle(Spec.Handle);
			// 새로 실행된 이 녀석도 태그만 떼고 즉시 종료
			K2_EndAbility();
			return;
		}
	}

	// [첫 실행 로직]
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		K2_EndAbility();
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UE_LOG(LogTemp, Log, TEXT("[SunFire] 토글 ON: 장판 시작"));
	
	

	// 시작 Cue (한 번만 실행)
	if (SunFireStartCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = GetAvatarActorFromActorInfo()->GetActorLocation();
		ASC->ExecuteGameplayCue(SunFireStartCueTag, Params);
	}

	// 루프 Cue (상태 유지)
	ASC->AddGameplayCue(SunFireLoopCueTag);

	// [추가/수정] 타이머 시작: 1초마다 ApplySunFireDamage를 호출해 주변 적을 찾습니다.
	// 반복 데미지 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(TickTimerHandle, this, &USSRSunFire::ApplySunFireDamage, TickInterval, true);
	
	// // GE 적용
	// if (DamageEffectClass)
	// {
	// 	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	// 	EffectContext.AddSourceObject(this);
	// 	
	// 	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, EffectContext);
	// }
}

void USSRSunFire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogTemp, Log, TEXT("[SunFire] EndAbility 호출됨 (토글 OFF)"));

	// 1. 타이머 정리
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TickTimerHandle);
	}

	// 2. Cue 정리
	if (ActorInfo->AbilitySystemComponent.IsValid())
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		ASC->RemoveGameplayCue(SunFireLoopCueTag);

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
	ACharacter* MyCharacter = Cast<ACharacter>(OwnerActor);
	
	if (!OwnerActor|| !MyASC || !MyCharacter) return;

	TArray<AActor*> OverlappedActors;
	GetActorInRange(OverlappedActors);

	for (AActor* Target : OverlappedActors)
	{
		if (Target == OwnerActor) continue;

		// 타겟이 CharaterBase인지 확인
		AGasCharacterBase* TargetCharacterBase = Cast<AGasCharacterBase>(Target);
		if (TargetCharacterBase && MyASC)
		{
			// 1.장판 데미지 정보
			FFuriDamageInfo DamageInfo;
			DamageInfo.Amount = -1.0f;
			DamageInfo.DamageType = EFuriDamageType::Melee;
			DamageInfo.DamageResponse = EFuriDamageResponse::None;
			
			// 가드 불가
			DamageInfo.bCanBeBlocked = false;
			
			if (DamageEffectClass)
			{
				FGameplayEffectContextHandle Context = MyASC->MakeEffectContext();
				FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, Context);
                
				if (SpecHandle.IsValid())
				{
					// SetByCaller로 대미지 수치 전달 (사용하시는 태그에 맞게 수정)
					SpecHandle.Data.Get()->SetSetByCallerMagnitude(
						FGameplayTag::RequestGameplayTag(FName("Data.Damage.TickAmount")), 
						DamageInfo.Amount
					);

					// 3. 타겟의 커스텀 함수 호출! (구조체 + GE 전달)
					TargetCharacterBase->TakeFuriDamage(DamageInfo, SpecHandle, OwnerActor);
                    
					// 4. (선택사항) 틱마다 가벼운 히트 이펙트나 사운드 실행
					FGameplayCueParameters HitParams;
					HitParams.Location = Target->GetActorLocation();
					MyASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.VFX.Hit")), HitParams);
				}
			}
			
		}
		
		
		
		
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
		{
			FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
			Context.AddSourceObject(OwnerActor);

			FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, Context);
			if (SpecHandle.IsValid())
			{
				TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				UE_LOG(LogTemp, Verbose, TEXT("[SunFire] %s에게 데미지 적용"), *Target->GetName());
			}
		}
	}
}

void USSRSunFire::GetActorInRange(TArray<AActor*>& OutActors)
{
	AActor* OwnerActor = GetAvatarActorFromActorInfo();
	if (!OwnerActor) return;

	FVector Center = OwnerActor->GetActorLocation();
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerActor);
	
	// 1. 모든 채널을 다 훑어서 주변에 '뭐라도' 있는지 확인 (디버그용)
	bool bHit = GetWorld()->OverlapMultiByChannel(
		Overlaps, 
		Center, 
		FQuat::Identity, 
		ECC_Visibility, // Pawn 대신 Visibility로 넓게 잡기
		FCollisionShape::MakeSphere(Radius), 
		Params
	);
	
	// 2. 디버그 구체 그리기 (영구적으로 남게 설정해서 범위를 눈으로 확인)
	DrawDebugSphere(GetWorld(), Center, Radius, 16, FColor::Red, false, 1.0f, 0, 2.0f);

	if (!bHit)
	{
		UE_LOG(LogTemp, Error, TEXT("[SunFire] 범위 내에 아무 액터도 감지되지 않음!"));
		return;
	}

	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* HitActor = Result.GetActor();
		if (HitActor)
		{
			// 감지된 모든 액터의 이름과 채널을 로그로 찍음
			UE_LOG(LogTemp, Warning, TEXT("[SunFire] 감지됨: %s / 콜리전 타입: %d"), 
				*HitActor->GetName(), (int32)HitActor->GetRootComponent()->GetCollisionObjectType());
            
			OutActors.AddUnique(HitActor);
		}
	}
	
	//////////////////////////////

	GetWorld()->OverlapMultiByChannel(
		Overlaps,
		Center,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(Radius),
		Params
		);

	// 디버그 구체 (에디터에서 확인용)
	DrawDebugSphere(GetWorld(), Center, Radius, 16, FColor::Orange, false, TickInterval);

	for (const FOverlapResult& Result : Overlaps)
	{
		if (AActor* HitActor = Result.GetActor())
		{
			OutActors.AddUnique(HitActor);
		}
	}
}

