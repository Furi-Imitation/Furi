// Fill out your copyright notice in the Description page of Project Settings.


#include "BasicAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/utils/FuriTypes.h"
#include "Net/UnrealNetwork.h"

UBasicAttributeSet::UBasicAttributeSet()
{
	Health = 100.f;
	MaxHealth = 100.f;
	Stamina = 100.f;
	MaxStamina = 100.f;
}

void UBasicAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
}

void UBasicAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
}

void UBasicAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
    
	// 🌟 [디테일 1] 체력이 변경되었을 때
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// 기존의 SetHealth(GetHealth())를 더 안전하게 바꿉니다.
		// 체력이 0 밑으로 떨어지거나, 최대 체력을 넘지 않도록 제한(Clamp)합니다.
		// (만약 GetMaxHealth()가 없다면 100.0f 같은 고정 수치나 변수를 넣으시면 됩니다.)
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, 100.0f /* GetMaxHealth() */));

		// 🌟 [핵심 로직] 공격자가 보낸 '피격 반응' 정보 읽기
		// 공격 어빌리티(UGA_Attack)에서 SetByCallerMagnitude로 보냈던 그 태그 값을 여기서 꺼냅니다.
		float ResponseValue = Data.EffectSpec.GetSetByCallerMagnitude(
			FGameplayTag::RequestGameplayTag(FName("Data.Damage.Response")), 
			false, // 태그를 못 찾아도 에러를 띄우지 않음 (일반 회복 포션 등을 먹었을 땐 이 태그가 없을 테니까요)
			0.0f   // 태그가 없을 경우 기본값은 0 (EFuriDamageResponse::None)
		);

		// 🌟 [디테일 2] 숫자를 다시 Enum 구조체로 변환
		EFuriDamageResponse DamageResponse = static_cast<EFuriDamageResponse>(FMath::RoundToInt(ResponseValue));

		// 피격 반응이 None(0)이 아니라면, 캐릭터에게 연출을 지시합니다.
		if (DamageResponse != EFuriDamageResponse::None)
		{
			AActor* TargetActor = Data.Target.GetAvatarActor();
    
			// EffectContext에서 이 공격을 가한 주체(Attacker)를 가져옵니다.
			AActor* Attacker = Data.EffectSpec.GetContext().GetInstigator();
    
			if (AGasCharacterBase* Victim = Cast<AGasCharacterBase>(TargetActor))
			{
				// 공격자 정보를 함께 넘겨줍니다!
				Victim->HandleDamageResponse(DamageResponse, Attacker);
			}
		}
	}
	// 스태미나가 변경되었을 때
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		// 스태미나 역시 0 이하로 떨어지지 않게 클램핑해줍니다.
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, 100.0f /* GetMaxStamina() */));
	}
}
