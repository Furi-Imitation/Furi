#include "BasicAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/utils/FuriTypes.h"
#include "Furi/GamePlayAbilitySystem/FuriAbilityTypes.h"
#include "Net/UnrealNetwork.h"

/**
 * 생성자: 게임이 시작될 때 어트리뷰트의 기본값을 설정합니다.
 */
UBasicAttributeSet::UBasicAttributeSet()
{
    // 캐릭터가 처음 생성될 때 가질 기본 스탯들입니다.
    Health = 100.f;
    MaxHealth = 100.f;
    Stamina = 100.f;
    MaxStamina = 100.f;
}

/**
 * GetLifetimeReplicatedProps: 멀티플레이어 환경에서 변수 동기화 방식을 정의합니다.
 * 서버에서 값이 변하면 클라이언트에게도 해당 값을 전달하도록 설정합니다.
 */
void UBasicAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // DOREPLIFETIME_CONDITION_NOTIFY 설명:
    // 1. COND_None: 조건 없이 모든 클라이언트에게 동기화합니다.
    // 2. REPNOTIFY_Always: 값이 동일하더라도 서버에서 설정되면 클라이언트의 RepNotify 함수를 항상 실행합니다.
    
    DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
}

/**
 * PreAttributeChange: 어트리뷰트의 값이 "실제로 변하기 직전"에 호출됩니다.
 * 주로 UI 업데이트 전이나 시스템 계산 중에 값이 비정상적인 범위로 튀지 않도록 '가이드라인'을 잡는 역할을 합니다.
 */
void UBasicAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    
    // Health가 변하려고 할 때: 0 ~ MaxHealth 사이로 제한(Clamp)
    if (Attribute == GetHealthAttribute())
    {
       NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
    }
    // MaxHealth가 변하려고 할 때: 최소 1 이상은 유지하도록 설정 (0이 되면 계산 오류 발생 가능성 방지)
    else if (Attribute == GetMaxHealthAttribute())
    {
       NewValue = FMath::Max(NewValue, 1.f);
    }
    // Stamina가 변하려고 할 때: 0 ~ MaxStamina 사이로 제한
    else if (Attribute == GetStaminaAttribute())
    {
       NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
    }
    // MaxStamina가 변하려고 할 때: 최소 1 이상 유지
    else if (Attribute == GetMaxStaminaAttribute())
    {
       NewValue = FMath::Max(NewValue, 1.f);
    }
}


/*
 *"어떤 Gameplay Effect(데미지, 힐 등)가 실제로 어트리뷰트(체력, 마나 등)의 값을 변화시킨 직후"에 호출되는 사후 처리 함수입니다.
 */
void UBasicAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	AActor* SourceActor = Context.GetOriginalInstigator();
	AActor* TargetActor = Data.Target.GetAvatarActor();
	AGasCharacterBase* TargetCharacter = Cast<AGasCharacterBase>(TargetActor);
    
	// =========================================================
	// 🩸 1. 체력(Health) 데이터 처리
	// =========================================================
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
       
		if (GetHealth() <= 0.0f)
		{
			if (TargetCharacter)
			{
				UE_LOG(LogTemp, Warning, TEXT("%s is Dead!"), *TargetCharacter->GetName());
			}
		}
		else
		{
			// 데미지를 입었을 때의 리액션 처리 (커스텀 컨텍스트 활용)
			if (FFuriGameplayEffectContext* FuriContext = FFuriGameplayEffectContext::GetFuriContext(Context))
			{
				const FFuriDamageInfo& DamageInfo = FuriContext->GetDamageInfo();
				if (TargetCharacter)
				{
					TargetCharacter->HandleDamageResponse(DamageInfo.DamageResponse, SourceActor);
				}
			}
		}
	}
	// =========================================================
	// ⚡ 2. 스태미나(Stamina) 데이터 처리
	// =========================================================
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}
}
