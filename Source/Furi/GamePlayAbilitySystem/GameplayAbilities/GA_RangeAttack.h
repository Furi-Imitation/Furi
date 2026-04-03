#pragma once

#include "CoreMinimal.h"
#include "FuriGameplayAbilityBase.h"
#include "Abilities/GameplayAbility.h"
#include "GA_RangeAttack.generated.h"

UCLASS()
class FURI_API UGA_RangeAttack : public UFuriGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_RangeAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnHitEventReceived(FGameplayEventData Payload);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Range")
	UAnimMontage* RangeAttackMontage;

	// 🌟 다른 공격 어빌리티와 이름을 맞추고 공용 데미지 GE를 사용합니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Range")
	TSubclassOf<UGameplayEffect> BaseDamageEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Range")
	float AttackRadius = 500.0f;

	// 🌟 C++에서 하드코딩하지 않고, 블루프린트 디테일 패널에서 지정할 태그들
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Cues")
	FGameplayTag HitEventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Cues")
	FGameplayTag StartCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Cues")
	FGameplayTag HitCueTag;
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Tags")
	FGameplayTag AttackVFXCueTag;
};
