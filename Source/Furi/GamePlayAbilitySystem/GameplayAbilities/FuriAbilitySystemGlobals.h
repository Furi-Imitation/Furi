#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "FuriAbilitySystemGlobals.generated.h"

UCLASS()
class FURI_API UFuriAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

public:
	// 🌟 엔진이 컨텍스트를 생성할 때 우리 것을 만들도록 가로채기(Override)
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};