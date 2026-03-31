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
	// 내 캐릭터의 Data Asset에서 현재 스킬의 데이터를 가져오는 헬퍼 함수
	bool GetCurrentSkillData(FFuriSkillData& OutData) const;

	// 🌟 GAS 코스트 적용 로직 가로채기
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	// 🌟 GAS 쿨타임 적용 로직 가로채기
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
};
