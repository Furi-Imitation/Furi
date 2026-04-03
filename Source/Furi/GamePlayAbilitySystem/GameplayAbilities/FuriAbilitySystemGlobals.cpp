#include "FuriAbilitySystemGlobals.h"
#include "Furi/GamePlayAbilitySystem/FuriAbilityTypes.h" // FFuriGameplayEffectContext가 정의된 헤더

FGameplayEffectContext* UFuriAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	// 엔진이 컨텍스트를 요청하면, 우리가 만든 커스텀 컨텍스트를 던져줍니다.
	return new FFuriGameplayEffectContext();
}