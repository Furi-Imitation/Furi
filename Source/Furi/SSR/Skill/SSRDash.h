// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SSRDash.generated.h"

/**
 * 
 */
UCLASS()
class FURI_API USSRDash : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	USSRDash();
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	FVector GetDashDirection() const;

protected:
	UFUNCTION()
	void OnDashFinished();
	
	UFUNCTION()
	FVector GetDashDirection();
	
	UPROPERTY(EditAnywhere, Category = "Dash")
	float DashStrength = 2000.f;
	
	UPROPERTY(EditAnywhere, Category = "Dash")
	float DashDuration = 0.3f;
	
	// 대시 시작 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Cues")
	FGameplayTag DashStartCueTag;
	// 대시 종료 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Cues")
	FGameplayTag DashEndCueTag;
	
	// 대쉬중 지속(잔상, 효과)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Cues")
	FGameplayTag DashVisualCueTag;
	
protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	
	
};
