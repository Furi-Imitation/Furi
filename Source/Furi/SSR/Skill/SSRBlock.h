// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Furi/GamePlayAbilitySystem/GameplayAbilities/FuriGameplayAbilityBase.h"
#include "SSRBlock.generated.h"

/**
 * SSR 캐릭터의 방어 어빌리티
 * - 버튼을 누르고 있는 동안 방어 상태 유지 (GE 부여)
 * - 방어 중 적의 공격이 적중하면 OnBlockSuccess 호출 (Event.Hit.Block)
 */
UCLASS()
class FURI_API USSRBlock : public UFuriGameplayAbilityBase
{
	GENERATED_BODY()

public:
	USSRBlock();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

protected:
	// 방어 상태(GE)가 제거되었을 때 호출
	UFUNCTION()
	void OnBlockEffectRemoved(const FGameplayEffectRemovalInfo& InGameplayEffectRemovalInfo);

	// 애니메이션이 중단되거나 완료되었을 때 호출 (매개변수 없음)
	UFUNCTION()
	void OnBlockEffectRemovedManual();

	// 입력이 해제되었을 때 호출
	UFUNCTION()
	void OnInputReleased(float TimeHeld);
	

	// 방어 상태(무적, 데미지 감소 등)를 부여할 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block")
	TSubclassOf<UGameplayEffect> BlockEffectClass;

	// 블락 애니메이션 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block")
	UAnimMontage* BlockMontage;

	// 블락 시작/종료/지속 Cue
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block|Cues")
	FGameplayTag BlockStartCueTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block|Cues")
	FGameplayTag BlockEndCueTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block|Cues")
	FGameplayTag BlockVisualCueTag;

private:
	FActiveGameplayEffectHandle ActiveBlockEffectHandle;
	bool bIsEnding = false;
	bool bBlockTriggered = false;
};
