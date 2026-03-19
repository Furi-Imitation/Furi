#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Dash.generated.h"

UCLASS()
class FURI_API UGA_Dash : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Dash();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 대시 힘 (수치가 높을수록 멀리 이동)
	UPROPERTY(EditAnywhere, Category = "Dash")
	float DashStrength = 3000.f;

	// 대시 중 무적 상태를 부여할 Gameplay Effect
	UPROPERTY(EditAnywhere, Category = "Dash")
	TSubclassOf<UGameplayEffect> InvincibleEffectClass;

	// 대시 시작 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Cues")
	FGameplayTag DashStartCueTag;
	// 대시 종료 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Cues")
	FGameplayTag DashEndCueTag;

	UFUNCTION()
	void OnDashFinished();
};