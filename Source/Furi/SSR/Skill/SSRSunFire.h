// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Furi/GamePlayAbilitySystem/GameplayAbilities/FuriGameplayAbilityBase.h"
#include "SSRSunFire.generated.h"

/**
 * 플레이어가 스킬을 활성화 하면 주변에 동그란 범위를 그린다 (반지름 1000)
 * 범위 안에 있으면 일정 주기마다 데미지를 입힌다. (가드 불가)
 * 스킬이 활성화 된 상태에서 다시 누르면 스킬이 비활성화 된다.
 */
UCLASS()
class FURI_API USSRSunFire : public UFuriGameplayAbilityBase
{
	GENERATED_BODY()

public:
	USSRSunFire();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                          const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	UFUNCTION()
	void ApplySunFireDamage();

	UFUNCTION()
	void GetActorInRange(TArray<AActor*>& OutActors);

	// 복잡한 Map 대신, 단 하나의 대미지 전용 공용 GE를 사용합니다. (Set by Caller 연동용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Damage")
	TSubclassOf<class UGameplayEffect> BaseDamageEffectClass;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SunFire | Balance", meta = (AllowPrivateAccess = "true"))
	float Radius = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SunFire | Balance", meta = (AllowPrivateAccess = "true"))
	float TickInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SunFire | Effects", meta = (AllowPrivateAccess = "true"))
	FGameplayTag SunFireStartCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SunFire | Effects", meta = (AllowPrivateAccess = "true"))
	FGameplayTag SunFireLoopCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SunFire | Effects", meta = (AllowPrivateAccess = "true"))
	FGameplayTag SunFireEndCueTag;

	FTimerHandle TickTimerHandle;
};
