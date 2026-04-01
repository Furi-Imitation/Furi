#pragma once

#include "CoreMinimal.h"
#include "FuriGameplayAbilityBase.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Ultimate.generated.h"

UCLASS()
class FURI_API UGA_Ultimate : public UFuriGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_Ultimate();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

protected:
	UFUNCTION()
	void OnHitEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageFinished();

	/** 🌟 물리 판정 및 대미지 로직 통합 함수 */
	void ProcessPhysicalHit();

	UPROPERTY(EditAnywhere, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> UltimateMontage;

	/** 🌟 물리 판정 범위 설정 */
	UPROPERTY(EditAnywhere, Category = "Combat|Collision")
	float AttackRange = 250.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Collision")
	float AttackBoxHalfWidth = 100.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Collision")
	float AttackBoxHalfHeight = 100.f;

	// 🌟 통일성을 위해 변수명 변경 (DamageEffectClass -> BaseDamageEffectClass)
	UPROPERTY(EditAnywhere, Category = "Combat|Damage")
	TSubclassOf<class UGameplayEffect> BaseDamageEffectClass;

	// --- Cues & Tags ---
	UPROPERTY(EditAnywhere, Category = "Combat|Tags")
	FGameplayTag HitEventTag;

	UPROPERTY(EditAnywhere, Category = "Combat|Tags")
	FGameplayTag HitVFXCueTag;

	UPROPERTY(EditAnywhere, Category = "Combat|Tags")
	FGameplayTag HitCameraShakeCueTag;

	UPROPERTY(EditAnywhere, Category = "Combat|Tags")
	FGameplayTag HitSFXSmallCueTag;

	UPROPERTY(EditAnywhere, Category = "Combat|Tags")
	FGameplayTag HitSFXLargeCueTag;

	// 🌟 궁극기 도중 내 앞에 고정해둘 타겟
	UPROPERTY()
	TObjectPtr<AActor> GrabbedTarget;

private:
	bool bFirstHitSuccess = false;
	int32 CurrentHitCount = 0;
};
