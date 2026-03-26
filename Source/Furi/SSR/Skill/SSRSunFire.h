// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SSRSunFire.generated.h"

/**
플레이어가 스킬을 활성화 하면
플레이어 주변에 동그란 범위를 그린다
그 범위 안에 있으면 초당 1씩 데미지를 입힌다. (가드 불가)
범위는 1000
스킬이 활성화 된 상태에서 다시 누르면 스킬이 비활성화 된다
 
 */
UCLASS()
class FURI_API USSRSunFire : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	USSRSunFire();
	
	// 스킬이 실행될때
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
		) override;

	// 스킬이 종료될때
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bwasCancelled
		) override;
	
protected:
	// 범위 1000
	UPROPERTY(EditAnywhere, Category = "SunFire")
	float Radius = 1000.f;
	
	// 틱당 데미지
	UPROPERTY(EditAnywhere, Category = "SunFire")
	float DamagePerTick = 1.0f;
	
	// 틱 간격 (1초)
	UPROPERTY(EditAnywhere, Category = "SunFire")
	float TickInterval = 1.0f;
	
	// 데미지 GameplayEffect
	
	UPROPERTY(EditAnywhere, Category = "SunFire")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	// Gameplay Cue (연출)
	
	// 시작
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SunFire")
	FGameplayTag SunFireStartCueTag;
	
	// 진행중
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SunFire")
	FGameplayTag SunFireLoopCueTag;
	
	// 종료
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SunFire")
	FGameplayTag SunFireEndCueTag;
	
	// 내부 상태
	// 활성화 상태
	bool bIsActive = false;
	
	FTimerHandle TickTimerHandle;
	
	// 내부 함수
	UFUNCTION()
	void ApplySunFireDamage();
	UFUNCTION()
	void GetActorInRange(TArray<AActor*>& OutActors);
	
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
};
