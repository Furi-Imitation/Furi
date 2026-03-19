#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Block.generated.h"

UCLASS()
class FURI_API UGA_Block : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Block();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	
	UFUNCTION()
	void OnBlockEffectRemoved(const FGameplayEffectRemovalInfo& InGameplayEffectRemovalInfo);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block")
	UAnimMontage* BlockMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block")
	TSubclassOf<UGameplayEffect> BlockEffectClass;

	//방어 시각/청각 효과를 위한 Cue 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block|Cues")
	FGameplayTag BlockStartCueTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block|Cues")
	FGameplayTag BlockEndCueTag;
	
	FActiveGameplayEffectHandle ActiveBlockEffectHandle;
};