#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystemComponent.h"
#include "FuriGameHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UHorizontalBox;

UCLASS()
class FURI_API UFuriGameHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void InitPlayerStats(UAbilitySystemComponent* InPlayerASC);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void InitEnemyStats(UAbilitySystemComponent* InEnemyASC);

protected:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_PlayerHP;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_PlayerStamina;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_EnemyHP;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_EnemyStamina;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_GameTime;

	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* Box_SkillContainer;

	void OnPlayerHealthChanged(const FOnAttributeChangeData& Data);
	void OnPlayerStaminaChanged(const FOnAttributeChangeData& Data);
	void OnEnemyHealthChanged(const FOnAttributeChangeData& Data);
	void OnEnemyStaminaChanged(const FOnAttributeChangeData& Data);

	// 🌟 스킬 아이콘을 동적으로 생성해서 컨테이너에 넣는 재사용 함수
	void UpdateSkillUI(UAbilitySystemComponent* TargetASC, UHorizontalBox* TargetContainer);

public:
	// 타이머 시작 함수
	UFUNCTION(BlueprintCallable, Category = "UI")
	void StartGameTimer();

private:
	TWeakObjectPtr<UAbilitySystemComponent> PlayerASC;
	TWeakObjectPtr<UAbilitySystemComponent> EnemyASC;

	bool bIsTimerRunning = false;
	float GameStartTime = 0.f;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UFuriSkillIconWidget> SkillIconWidgetClass;
};
