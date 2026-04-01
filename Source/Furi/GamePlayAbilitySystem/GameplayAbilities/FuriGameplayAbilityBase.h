#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Furi/utils/FuriSkillDataAsset.h"
#include "FuriGameplayAbilityBase.generated.h"

UCLASS()
class FURI_API UFuriGameplayAbilityBase : public UGameplayAbility
{
	GENERATED_BODY()

protected:

	// 🌟 GAS 코스트 적용 로직 가로채기
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	// 🌟 GAS 쿨타임 적용 로직 가로채기
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	// 🌟 엔진의 기본 검사 로직을 무시하고 직접 검사하기 위해 오버라이드
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	
	// 🌟 InActorInfo를 매개변수로 받을 수 있도록 추가 (기본값 nullptr)
	bool GetCurrentSkillData(FFuriSkillData& OutData, const FGameplayAbilityActorInfo* InActorInfo = nullptr) const;
};
