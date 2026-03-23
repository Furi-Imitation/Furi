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
    
	// =========================================================
	// 🩸 1. 체력(Health) 데이터 처리
	// =========================================================
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// 체력이 0 밑으로 떨어지거나, 최대 체력을 넘지 않도록 안전하게 제한(Clamp)합니다.
		// (추후 MaxHealth 어트리뷰트를 만드시면 100.0f 대신 GetMaxHealth()로 교체하세요.)
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, 100.0f));
       
		if (GetHealth() <= 0.0f)
		{
			AActor* TargetActor = Data.Target.GetAvatarActor();
			if (AGasCharacterBase* Victim = Cast<AGasCharacterBase>(TargetActor))
			{
				// TODO: 캐릭터가 죽었을 때의 처리 (예: Victim->Die() 함수 호출)
				UE_LOG(LogTemp, Warning, TEXT("%s is Dead!"), *Victim->GetName());
               
				// 보통 여기서 사망 애니메이션을 틀거나, 충돌체를 끄는 로직을 호출합니다.
			}
		}
	}
	// =========================================================
	// ⚡ 2. 스태미나(Stamina) 데이터 처리
	// =========================================================
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		// 스태미나 역시 0 이하로 떨어지지 않게 클램핑해줍니다.
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, 100.0f));
	}
}
