#include "FuriGameHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Furi/GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"

void UFuriGameHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 🌟 [게임 타이머 업데이트]
    if (Text_GameTime && GetWorld())
    {
        // 월드의 현재 게임 진행 시간을 초 단위로 가져옵니다.
        int32 TotalSeconds = FMath::FloorToInt(GetWorld()->GetTimeSeconds());
        
        int32 Minutes = TotalSeconds / 60;
        int32 Seconds = TotalSeconds % 60;

        // "04:12" 형식으로 문자열 포맷팅
        FString TimeString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
        Text_GameTime->SetText(FText::FromString(TimeString));
    }
}

void UFuriGameHUDWidget::InitPlayerStats(UAbilitySystemComponent* InPlayerASC)
{
    if (!InPlayerASC) return;
    PlayerASC = InPlayerASC;

    // 1. 초기 UI 값 세팅 시 Max 속성으로 나누기
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

    // 2. 델리게이트 연결
    PlayerASC->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetHealthAttribute()).AddUObject(this, &UFuriGameHUDWidget::OnPlayerHealthChanged);
    PlayerASC->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetStaminaAttribute()).AddUObject(this, &UFuriGameHUDWidget::OnPlayerStaminaChanged);
}

void UFuriGameHUDWidget::InitEnemyStats(UAbilitySystemComponent* InEnemyASC)
{
    if (!InEnemyASC) return;
    EnemyASC = InEnemyASC;

    // 1. 초기 UI 값 세팅
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
    
    // 2. 델리게이트 연결
    EnemyASC->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetHealthAttribute()).AddUObject(this, &UFuriGameHUDWidget::OnEnemyHealthChanged);
    EnemyASC->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetStaminaAttribute()).AddUObject(this, &UFuriGameHUDWidget::OnEnemyStaminaChanged);
}

void UFuriGameHUDWidget::OnPlayerHealthChanged(const FOnAttributeChangeData& Data)
{
    if (PB_PlayerHP && PlayerASC.IsValid())
    {
        // 실시간으로 MaxHealth를 가져와서 퍼센트를 계산합니다.
        float MaxHP = PlayerASC->GetNumericAttribute(UBasicAttributeSet::GetMaxHealthAttribute());
        float HealthPercent = MaxHP > 0.f ? (Data.NewValue / MaxHP) : 0.f;
        PB_PlayerHP->SetPercent(HealthPercent);
    }
}

void UFuriGameHUDWidget::OnPlayerStaminaChanged(const FOnAttributeChangeData& Data)
{
    if (PB_PlayerStamina && PlayerASC.IsValid())
    {
        float MaxStamina = PlayerASC->GetNumericAttribute(UBasicAttributeSet::GetMaxStaminaAttribute());
        float StaminaPercent = MaxStamina > 0.f ? (Data.NewValue / MaxStamina) : 0.f;
        PB_PlayerStamina->SetPercent(StaminaPercent);
    }
}

void UFuriGameHUDWidget::OnEnemyHealthChanged(const FOnAttributeChangeData& Data)
{
    if (PB_EnemyHP && EnemyASC.IsValid())
    {
        float MaxHP = EnemyASC->GetNumericAttribute(UBasicAttributeSet::GetMaxHealthAttribute());
        float HealthPercent = MaxHP > 0.f ? (Data.NewValue / MaxHP) : 0.f;
        PB_EnemyHP->SetPercent(HealthPercent);
    }
}

void UFuriGameHUDWidget::OnEnemyStaminaChanged(const FOnAttributeChangeData& Data)
{
    if (PB_EnemyStamina && EnemyASC.IsValid())
    {
        float MaxStamina = EnemyASC->GetNumericAttribute(UBasicAttributeSet::GetMaxStaminaAttribute());
        float StaminaPercent = MaxStamina > 0.f ? (Data.NewValue / MaxStamina) : 0.f;
        PB_EnemyStamina->SetPercent(StaminaPercent);
    }
}