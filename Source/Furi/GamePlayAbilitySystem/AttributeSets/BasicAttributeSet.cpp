#include "BasicAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/GamePlayAbilitySystem/FuriAbilityTypes.h"
#include "Net/UnrealNetwork.h"

UBasicAttributeSet::UBasicAttributeSet()
{
	// 캐릭터가 처음 생성될 때 가질 기본 스탯들입니다.
	Health = 500.f;
	MaxHealth = 500.f;
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
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
}

void UBasicAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	AActor* SourceActor = Context.GetOriginalInstigator();
	AActor* TargetActor = Data.Target.GetAvatarActor();
	AGasCharacterBase* TargetCharacter = Cast<AGasCharacterBase>(TargetActor);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));

		if (GetHealth() <= 0.0f)
		{
			if (TargetCharacter)
			{
				UE_LOG(LogTemp, Warning, TEXT("[BasicAttributeSet] %s is Dead!"), *TargetCharacter->GetName());
				TargetCharacter->Die();
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[BasicAttributeSet] Health changed. Current: %.2f"), GetHealth());

			// 데미지를 입었을 때의 리액션 처리 (커스텀 컨텍스트 활용)
			FFuriGameplayEffectContext* FuriContext = FFuriGameplayEffectContext::GetFuriContext(Context);
			if (FuriContext)
			{
				const FFuriDamageInfo& DamageInfo = FuriContext->GetDamageInfo();
				if (TargetCharacter)
				{
					UE_LOG(LogTemp, Warning, TEXT("[BasicAttributeSet] Calling HandleDamageResponse for %s"),
					       *TargetCharacter->GetName());
					TargetCharacter->HandleDamageResponse(DamageInfo, SourceActor);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[BasicAttributeSet] TargetCharacter is NULL!"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error,
				       TEXT(
					       "[BasicAttributeSet] FuriContext is NULL! Check if the GE is using FFuriGameplayEffectContext."
				       ));
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}
}
