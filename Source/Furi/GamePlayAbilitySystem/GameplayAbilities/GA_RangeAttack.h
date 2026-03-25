#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_RangeAttack.generated.h"

UCLASS()
class FURI_API UGA_RangeAttack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_RangeAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	UFUNCTION()
	void OnMontageFinished();
	// 애님 노티파이(Event)를 받았을 때 실행될 함수
	UFUNCTION()
	void OnHitEventReceived(FGameplayEventData Payload);

protected:
	UPROPERTY(EditAnywhere, Category = "Combat|Range")
	UAnimMontage* RangeAttackMontage;

	UPROPERTY(EditAnywhere, Category = "Combat|Range")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditAnywhere, Category = "Combat|Range")
	float AttackRadius = 500.0f;

	// 노티파이에서 던져줄 태그
	UPROPERTY(EditAnywhere, Category = "Combat|Range")
	FGameplayTag HitEventTag;

	UPROPERTY(EditAnywhere, Category = "Combat|Range")
	FGameplayTag StartCueTag;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Range")
	FGameplayTag HitCueTag;
};
