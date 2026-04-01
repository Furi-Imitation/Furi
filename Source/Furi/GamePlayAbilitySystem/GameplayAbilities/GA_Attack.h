#pragma once

#include "CoreMinimal.h"
#include "FuriGameplayAbilityBase.h"
#include "GameplayTagContainer.h"
#include "Sound/SoundBase.h"
#include "GA_Attack.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

UCLASS()
class FURI_API UGA_Attack : public UFuriGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_Attack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                          const FGameplayAbilityActivationInfo ActivationInfo) override;

	UFUNCTION()
	void OnComboEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	void PerformHitCheck();
	void PlayComboSection();
	void RotateTowardsClosestEnemy(AActor* MyActor, float SearchRadius);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Animation")
	UAnimMontage* ComboMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Animation")
	FGameplayTag HitCheckEventTag;

	// 복잡한 Map 대신, 단 하나의 대미지 전용 공용 GE를 사용합니다. (Set by Caller 연동용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Damage")
	TSubclassOf<class UGameplayEffect> BaseDamageEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitCheck")
	float AttackRange = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitCheck")
	float AttackBoxHalfWidth = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitCheck")
	float AttackBoxHalfHeight = 60.0f;

private:
	int32 CurrentComboIndex;
	bool bComboWindowOpened;
	bool bNextComboReserved;

	UPROPERTY()
	UAbilityTask_PlayMontageAndWait* MontageTask;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TObjectPtr<USoundBase> SwingSound;

	void PlaySwingSound();
};
