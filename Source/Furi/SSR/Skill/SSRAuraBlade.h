#pragma once

#include "CoreMinimal.h"
#include "Furi/GamePlayAbilitySystem/GameplayAbilities/FuriGameplayAbilityBase.h"
#include "SSRAuraBlade.generated.h"

UCLASS()
class FURI_API USSRAuraBlade : public UFuriGameplayAbilityBase
{
	GENERATED_BODY()

public:
	USSRAuraBlade();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
protected:
	UFUNCTION()
	void OnDelayFinished();

	// 대미지 전용 공용 GE를 사용합니다. (Set by Caller 연동용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Damage")
	TSubclassOf<class UGameplayEffect> BaseDamageEffectClass;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AuraBlade|Projectile",
		meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AAuraBladeProjectile> ProjectileClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AuraBlade|Balance", meta = (AllowPrivateAccess = "true"))
	float DelayTime = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AuraBlade|Animation",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> ChargeMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AuraBlade|Effects", meta = (AllowPrivateAccess = "true"))
	FGameplayTag ChargeCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AuraBlade|Effects", meta = (AllowPrivateAccess = "true"))
	FGameplayTag FireCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AuraBlade|Effects", meta = (AllowPrivateAccess = "true"))
	FGameplayTag HitCueTag;
};
