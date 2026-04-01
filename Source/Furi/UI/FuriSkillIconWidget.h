#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "FuriSkillIconWidget.generated.h"

struct FFuriSkillData;
class UProgressBar;
class UTextBlock;
class UImage;
class UAbilitySystemComponent;

UCLASS()
class FURI_API UFuriSkillIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 메인 HUD가 위젯을 생성할 때 호출해 줄 초기화 함수
	void InitSkillWidget(const FFuriSkillData& SkillData, UAbilitySystemComponent* InASC);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 🎨 UI 바인딩
	// 뒤에 깔릴 옅은 색상의 배경 아이콘
	UPROPERTY(meta = (BindWidget))
	UImage* Image_BackgroundIcon;

	// 밑에서 위로 차오를 프로그레스 바 (밝은 아이콘)
	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_CooldownOverlay;

	// 좌측 하단 스태미나 코스트 텍스트
	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_StaminaCost;

private:
	FGameplayTag CooldownTag;
	TWeakObjectPtr<UAbilitySystemComponent> ASC;
};
