#include "FuriGameHUDWidget.h"
#include "FuriSkillIconWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Furi/GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/utils/FuriSkillDataAsset.h"

void UFuriGameHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (Text_GameTime && GetWorld())
	{
		int32 TotalSeconds = FMath::FloorToInt(GetWorld()->GetTimeSeconds());
		int32 Minutes = TotalSeconds / 60;
		int32 Seconds = TotalSeconds % 60;

		FString TimeString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		Text_GameTime->SetText(FText::FromString(TimeString));
	}
}

void UFuriGameHUDWidget::InitPlayerStats(UAbilitySystemComponent* InPlayerASC)
{
	if (!InPlayerASC)
	{
		return;
	}
	PlayerASC = InPlayerASC;

	if (PB_PlayerHP)
	{
		float CurrentHP = PlayerASC->GetNumericAttribute(UBasicAttributeSet::GetHealthAttribute());
		float MaxHP = PlayerASC->GetNumericAttribute(UBasicAttributeSet::GetMaxHealthAttribute());
		PB_PlayerHP->SetPercent(MaxHP > 0.f ? (CurrentHP / MaxHP) : 0.f);
	}
	if (PB_PlayerStamina)
	{
		float CurrentStamina = PlayerASC->GetNumericAttribute(UBasicAttributeSet::GetStaminaAttribute());
		float MaxStamina = PlayerASC->GetNumericAttribute(UBasicAttributeSet::GetMaxStaminaAttribute());
		PB_PlayerStamina->SetPercent(MaxStamina > 0.f ? (CurrentStamina / MaxStamina) : 0.f);
	}

	PlayerASC->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetHealthAttribute()).AddUObject(
		this, &UFuriGameHUDWidget::OnPlayerHealthChanged);
	PlayerASC->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetStaminaAttribute()).AddUObject(
		this, &UFuriGameHUDWidget::OnPlayerStaminaChanged);

	// 🌟 내 캐릭터의 DataAsset을 읽어와서 Box_SkillContainer에 채웁니다.
	UpdateSkillUI(PlayerASC.Get(), Box_SkillContainer);
}

void UFuriGameHUDWidget::InitEnemyStats(UAbilitySystemComponent* InEnemyASC)
{
	if (!InEnemyASC)
	{
		return;
	}
	EnemyASC = InEnemyASC;

	if (PB_EnemyHP)
	{
		float CurrentHP = EnemyASC->GetNumericAttribute(UBasicAttributeSet::GetHealthAttribute());
		float MaxHP = EnemyASC->GetNumericAttribute(UBasicAttributeSet::GetMaxHealthAttribute());
		PB_EnemyHP->SetPercent(MaxHP > 0.f ? (CurrentHP / MaxHP) : 0.f);
	}
	if (PB_EnemyStamina)
	{
		float CurrentStamina = EnemyASC->GetNumericAttribute(UBasicAttributeSet::GetStaminaAttribute());
		float MaxStamina = EnemyASC->GetNumericAttribute(UBasicAttributeSet::GetMaxStaminaAttribute());
		PB_EnemyStamina->SetPercent(MaxStamina > 0.f ? (CurrentStamina / MaxStamina) : 0.f);
	}

	EnemyASC->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetHealthAttribute()).AddUObject(
		this, &UFuriGameHUDWidget::OnEnemyHealthChanged);
	EnemyASC->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetStaminaAttribute()).AddUObject(
		this, &UFuriGameHUDWidget::OnEnemyStaminaChanged);
}

// 🌟 스킬 위젯 동적 생성 헬퍼 함수
void UFuriGameHUDWidget::UpdateSkillUI(UAbilitySystemComponent* TargetASC, UHorizontalBox* TargetContainer)
{
	if (!TargetASC || !TargetContainer || !SkillIconWidgetClass)
	{
		return;
	}

	TargetContainer->ClearChildren();

	if (AGasCharacterBase* TargetChar = Cast<AGasCharacterBase>(TargetASC->GetAvatarActor()))
	{
		// 해당 캐릭터(내 캐릭터 OR 적 캐릭터)에 할당된 DataAsset을 가져옵니다.
		if (UFuriSkillDataAsset* SkillDA = TargetChar->GetSkillUIData())
		{
			for (const auto& Pair : SkillDA->SkillDataMap)
			{
				UFuriSkillIconWidget* IconWidget = CreateWidget<UFuriSkillIconWidget>(this, SkillIconWidgetClass);
				if (IconWidget)
				{
					IconWidget->InitSkillWidget(Pair.Value, TargetASC);

					UHorizontalBoxSlot* BoxSlot = TargetContainer->AddChildToHorizontalBox(IconWidget);
					if (BoxSlot)
					{
						BoxSlot->SetPadding(FMargin(0.f, 0.f, 15.f, 0.f));
					}
				}
			}
		}
	}
}

void UFuriGameHUDWidget::OnPlayerHealthChanged(const FOnAttributeChangeData& Data)
{
	if (PB_PlayerHP && PlayerASC.IsValid())
	{
		float MaxHP = PlayerASC->GetNumericAttribute(UBasicAttributeSet::GetMaxHealthAttribute());
		PB_PlayerHP->SetPercent(MaxHP > 0.f ? (Data.NewValue / MaxHP) : 0.f);
	}
}

void UFuriGameHUDWidget::OnPlayerStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (PB_PlayerStamina && PlayerASC.IsValid())
	{
		float MaxStamina = PlayerASC->GetNumericAttribute(UBasicAttributeSet::GetMaxStaminaAttribute());
		PB_PlayerStamina->SetPercent(MaxStamina > 0.f ? (Data.NewValue / MaxStamina) : 0.f);
	}
}

void UFuriGameHUDWidget::OnEnemyHealthChanged(const FOnAttributeChangeData& Data)
{
	if (PB_EnemyHP && EnemyASC.IsValid())
	{
		float MaxHP = EnemyASC->GetNumericAttribute(UBasicAttributeSet::GetMaxHealthAttribute());
		PB_EnemyHP->SetPercent(MaxHP > 0.f ? (Data.NewValue / MaxHP) : 0.f);
	}
}

void UFuriGameHUDWidget::OnEnemyStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (PB_EnemyStamina && EnemyASC.IsValid())
	{
		float MaxStamina = EnemyASC->GetNumericAttribute(UBasicAttributeSet::GetMaxStaminaAttribute());
		PB_EnemyStamina->SetPercent(MaxStamina > 0.f ? (Data.NewValue / MaxStamina) : 0.f);
	}
}
