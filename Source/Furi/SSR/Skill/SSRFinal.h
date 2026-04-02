// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SSRFinal.generated.h"

/**
 * 
 */
UCLASS()
class FURI_API USSRFinal : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USSRFinal();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 🌟 사용될 필살기 몽타주 (에디터에서 SSR_AM_FinalAttack 할당)
	UPROPERTY(EditAnywhere, Category = "Abilities")
	TObjectPtr<UAnimMontage> FinalAttackMontage;
	
	// 데미지 이펙트
	UPROPERTY(EditAnywhere, Category = "Abilities | Damage")
	TSubclassOf<class UGameplayEffect> DamageEffectClass;

	// 🌟 적중 성공 시 실행될 함수
	UFUNCTION()
	void OnHitSuccess(FGameplayEventData Payload);

	// 콤보 도중 데미지를 입히기 위해 호출할 함수
	UFUNCTION()
	void ApplyDamageToGrabbedTarget(float InDamageAmount);
	// 데미지 값 구현
	
	UFUNCTION()
	void OnDamageNotifyReceived(FGameplayEventData Payload);

	// 🌟 공격 판정 시작 노티파이를 받았을 때
	UFUNCTION()
	void OnActivateCollision(FGameplayEventData Payload);

	// 🌟 적중 실패(Trigger 섹션 종료) 시 실행될 함수
	UFUNCTION()
	void OnAttackFailed(FGameplayEventData Payload);
	
	// 🌟 몽타주가 끝났을 때 어빌리티를 종료시키기 위한 함수
	UFUNCTION()
	void OnMontageFinished();

private:
	// 중복 실행 방지 및 상태 체크용
	bool bHitConfirmed = false;
	
	int CurrentHitCount = 0;
	
	UPROPERTY()
	TObjectPtr<AActor> GrabbedTarget;
};
