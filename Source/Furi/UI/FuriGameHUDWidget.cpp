#include "FuriGameHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
// AttributeSet 헤더 포함 (예: FuriAttributeSet.h)
// #include "FuriAttributeSet.h" 

void UFuriGameHUDWidget::InitPlayerStats(UAbilitySystemComponent* InPlayerASC)
{
	if (!InPlayerASC)
	{
		return;
	}
	PlayerASC = InPlayerASC;

	// 🌟 GAS의 속성값이 변할 때마다 UI 함수가 자동으로 호출되도록 델리게이트(Delegate)를 연결합니다.
	// (UFuriAttributeSet은 프로젝트에 맞게 클래스명 변경 필요)
	/* PlayerASC->GetGameplayAttributeValueChangeDelegate(UFuriAttributeSet::GetHealthAttribute()).AddUObject(this, &UFuriGameHUDWidget::OnPlayerHealthChanged);
	PlayerASC->GetGameplayAttributeValueChangeDelegate(UFuriAttributeSet::GetStaminaAttribute()).AddUObject(this, &UFuriGameHUDWidget::OnPlayerStaminaChanged);
	*/

	// 초기값 세팅 로직 (현재 체력을 가져와서 프로그레스 바 갱신 등)
}

void UFuriGameHUDWidget::InitEnemyStats(UAbilitySystemComponent* InEnemyASC)
{
	if (!InEnemyASC)
	{
		return;
	}
	EnemyASC = InEnemyASC;

	/*
	EnemyASC->GetGameplayAttributeValueChangeDelegate(UFuriAttributeSet::GetHealthAttribute()).AddUObject(this, &UFuriGameHUDWidget::OnEnemyHealthChanged);
	*/
}

void UFuriGameHUDWidget::OnPlayerHealthChanged(const FOnAttributeChangeData& Data)
{
	if (PB_PlayerHP)
	{
		// MaxHealth를 가져오는 로직 추가 필요. 임시로 Max를 100으로 가정.
		float HealthPercent = Data.NewValue / 100.0f;
		PB_PlayerHP->SetPercent(HealthPercent);
	}
}

void UFuriGameHUDWidget::OnPlayerStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (PB_PlayerStamina)
	{
		float StaminaPercent = Data.NewValue / 100.0f;
		PB_PlayerStamina->SetPercent(StaminaPercent);
	}
}

void UFuriGameHUDWidget::OnEnemyHealthChanged(const FOnAttributeChangeData& Data)
{
	if (PB_EnemyHP)
	{
		float HealthPercent = Data.NewValue / 100.0f;
		PB_EnemyHP->SetPercent(HealthPercent);
	}
}

void UFuriGameHUDWidget::OnEnemyStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (PB_EnemyStamina)
	{
		float StaminaPercent = Data.NewValue / 100.0f;
		PB_EnemyStamina->SetPercent(StaminaPercent);
	}
}
