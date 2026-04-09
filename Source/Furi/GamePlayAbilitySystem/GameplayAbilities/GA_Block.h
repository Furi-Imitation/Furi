#pragma once

#include "CoreMinimal.h"
#include "FuriGameplayAbilityBase.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Block.generated.h"

class UAnimMontage;
class UGameplayEffect;

UCLASS()
class FURI_API UGA_Block : public UFuriGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_Block();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

	UFUNCTION()
	void OnBlockEffectRemoved(const FGameplayEffectRemovalInfo& InGameplayEffectRemovalInfo);

	UFUNCTION()
	void OnBlockMontageFinished();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block")
	UAnimMontage* BlockMontage;

	// 방어 상태(무적, 데미지 감소 등)를 부여할 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block")
	TSubclassOf<UGameplayEffect> BlockEffectClass;

	// 방어 시각/청각 효과를 위한 Cue 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block|Cues")
	FGameplayTag BlockStartCueTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block|Cues")
	FGameplayTag BlockEndCueTag;

private:
	FActiveGameplayEffectHandle ActiveBlockEffectHandle;
	bool bIsEnding; // 🌟 무한 루프 방지용 플래그
};
