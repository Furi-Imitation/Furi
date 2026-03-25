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
	// [기존의 TArray 검색 로직 과감히 삭제]

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		K2_EndAbility();
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
	// 시작 및 루프 이펙트, 타이머 실행 (기존 코드 유지)
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (SunFireStartCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = GetAvatarActorFromActorInfo()->GetActorLocation();
		ASC->ExecuteGameplayCue(SunFireStartCueTag, Params);
	}
	ASC->AddGameplayCue(SunFireLoopCueTag);

	GetWorld()->GetTimerManager().SetTimer(TickTimerHandle, this, &USSRSunFire::ApplySunFireDamage, TickInterval, true);
    
	UE_LOG(LogTemp, Log, TEXT("[SunFire] 장판 활성화됨"));
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
    if (!OwnerActor || !MyASC) return;

    TArray<AActor*> OverlappedActors;
    GetActorInRange(OverlappedActors);

    for (AActor* Target : OverlappedActors)
    {
        // 1. 나 자신은 제외
        if (Target == OwnerActor) continue;

        // 2. 타겟이 우리의 캐릭터 베이스인지 확인 (필터링)
        AGasCharacterBase* TargetCharacterBase = Cast<AGasCharacterBase>(Target);
        if (TargetCharacterBase && DamageEffectClass)
        {
            // 데미지 정보 조립
            FFuriDamageInfo DamageInfo;
            DamageInfo.Amount = -1.0f;
            DamageInfo.DamageType = EFuriDamageType::Melee; // 혹은 장판용 속성
            DamageInfo.DamageResponse = EFuriDamageResponse::None;
            DamageInfo.bCanBeBlocked = false; // 장판은 가드 불가 설정
            
            FGameplayEffectContextHandle Context = MyASC->MakeEffectContext();
            FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, Context);
                
            if (SpecHandle.IsValid())
            {
                // 🌟 [중요] 여기서 값을 정확히 주입!
                SpecHandle.Data.Get()->SetSetByCallerMagnitude(
                   FGameplayTag::RequestGameplayTag(FName("Data.Damage.TickAmount")), 
                   DamageInfo.Amount
                );

                // 3. 🌟 핵심: 타겟의 커스텀 함수 하나만 호출 (이 안에서 모든 판정이 끝남)
                TargetCharacterBase->TakeFuriDamage(DamageInfo, SpecHandle, OwnerActor);
                    
                // 4. 타격 비주얼 효과
                FGameplayCueParameters HitParams;
                HitParams.Location = Target->GetActorLocation();
                MyASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.P1.VFX.Hit")), HitParams);

                UE_LOG(LogTemp, Verbose, TEXT("[SunFire] %s에게 틱 데미지 적용 완료"), *Target->GetName());
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

// 다시 눌렀을 때 호출되는 InputPressed에서 종료 처리를 합니다.
void USSRSunFire::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	// 이미 실행 중인데 버튼이 다시 눌렸다면? -> 토글 OFF
	UE_LOG(LogTemp, Warning, TEXT("[SunFire] 다시 누름 감지 -> 토글 OFF (종료합니다)"));
    
	// 이 함수를 호출하면 EndAbility가 실행됩니다.
	K2_EndAbility(); 
}
