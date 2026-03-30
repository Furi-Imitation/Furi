// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SSRBlock.generated.h"

/**
 * 
 */
UCLASS()
class FURI_API USSRBlock : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	USSRBlock();
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
protected:
	// 방어 성공시 호출
	UFUNCTION()
	void OnBlockSuccess(FGameplayEventData Payload);
	
	// 1초 동안 블락을 했을때
	UFUNCTION()
	void OnBlockTimeout();
	
	// 1회 방어 성공 후 즉시 종료를 위한 플래그
	bool bBlockTriggered = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block|Config")
	float BlockMaxDuration = 1.0f;
	
	// 블락 애니메이션 몬티지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block")
	UAnimMontage* BlockMontage;

	// 블락 성공 후 Effect 선언
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block")
	TSubclassOf<UGameplayEffect> StunEffectClass;
	
	// 블락 시작 Cue
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block|Cues")
	FGameplayTag BlockStartCueTag;
	
	// 블락 종료 Cue
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block|Cues")
	FGameplayTag BlockEndCueTag;
};
