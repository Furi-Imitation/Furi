#pragma once

#include "CoreMinimal.h"
#include "FuriGameplayAbilityBase.h"
#include "Abilities/GameplayAbility.h"
#include "GA_TeleportDashAttack.generated.h"

UCLASS()
class FURI_API UGA_TeleportDashAttack : public UFuriGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_TeleportDashAttack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

	UFUNCTION()
	void PrepareNextStrike();

	UFUNCTION()
	void OnStrikeDelayFinished();

	UFUNCTION()
	void OnStrikeMontageFinished();

	UFUNCTION()
	void OnStrikeInterrupted();

	UFUNCTION()
	void OnHitCheckEventReceived(FGameplayEventData Payload);

	int32 CurrentStrikeCount;
	const int32 MaxStrikeCount = 3;

	UPROPERTY()
	AActor* LockedTarget;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	TArray<TObjectPtr<UAnimMontage>> StrikeMontages;

	// 🌟 다른 스킬들과 통일성을 위해 BaseDamageEffectClass로 명칭 변경
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage")
	TSubclassOf<class UGameplayEffect> BaseDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Teleport")
	float StrikeDelay = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitCheck")
	float AttackRange = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitCheck")
	float AttackBoxHalfWidth = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitCheck")
	float AttackBoxHalfHeight = 60.0f;

	// --- Cues & Tags (블루프린트에서 할당) ---
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Tags")
	FGameplayTag HitEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Tags")
	FGameplayTag VanishCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Tags")
	FGameplayTag TeleportCueTag;

	// 🌟 C++ 하드코딩 방지를 위해 추가된 태그 변수들
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Tags")
	FGameplayTag AttackVFXCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Tags")
	FGameplayTag CameraShakeCueTag;
};
