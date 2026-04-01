#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystemComponent.h"
#include "FuriGameHUDWidget.generated.h"

// 전방 선언 (헤더 포함 최소화를 통한 컴파일 속도 향상)
class UProgressBar;
class UTextBlock;
class UHorizontalBox;

UCLASS()
class FURI_API UFuriGameHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 🌟 플레이어 및 적의 ASC를 UI에 연결하는 초기화 함수
	UFUNCTION(BlueprintCallable, Category = "UI")
	void InitPlayerStats(UAbilitySystemComponent* InPlayerASC);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void InitEnemyStats(UAbilitySystemComponent* InEnemyASC);

protected:
	// ==========================================
	// 🎨 UI 컴포넌트 바인딩 (WBP의 이름과 정확히 일치해야 합니다)
	// ==========================================

	// [좌측 상단: 내 캐릭터]
	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_PlayerHP;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_PlayerStamina;

	// [우측 상단: 적 캐릭터]
	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_EnemyHP;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_EnemyStamina;

	// [중앙 상단: 플레이 타임]
	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_GameTime;

	// [중앙 하단: 스킬 쿨타임 및 정보 컨테이너]
	// 나중에 DataAsset을 읽어와서 이 Box 안에 스킬 아이콘 위젯들을 동적으로 생성해 넣을 것입니다.
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* Box_SkillContainer;

	// ==========================================
	// ⚙️ 내부 콜백 함수 (GAS 속성 변경 시 호출됨)
	// ==========================================
	void OnPlayerHealthChanged(const FOnAttributeChangeData& Data);
	void OnPlayerStaminaChanged(const FOnAttributeChangeData& Data);
	void OnEnemyHealthChanged(const FOnAttributeChangeData& Data);
	void OnEnemyStaminaChanged(const FOnAttributeChangeData& Data);

private:
	// 포인터 캐싱
	TWeakObjectPtr<UAbilitySystemComponent> PlayerASC;
	TWeakObjectPtr<UAbilitySystemComponent> EnemyASC;

protected:
	// 매 프레임 UI를 업데이트하기 위한 틱 함수 (타이머 용도)
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 에디터에서 방금 만든 개별 스킬 위젯(WBP_SkillIcon) 클래스를 할당할 변수
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UFuriSkillIconWidget> SkillIconWidgetClass;
};
