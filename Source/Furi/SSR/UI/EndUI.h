#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EndUI.generated.h"

UCLASS()
class FURI_API UEndUI : public UUserWidget
{
	GENERATED_BODY()

public:
	// UI가 생성될 때 승리/패배 여부를 결정하는 함수
	void SetGameResult(bool bIsVictory);
	
	// 블루프린트에서 구현할 이벤트 (C++에서는 정의하지 않고 호출만 함)
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Result")
	void OnGameResultDetermined(bool bIsVictory);

	// 🌟 블루프린트에서 페이드 인 애니메이션을 구현할 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Result")
	void PlayFadeInAnimation();

protected:
	virtual bool Initialize() override;

	// 블루프린트의 위젯들과 연결 (이름이 일치해야 함)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category="UI")
	class UTextBlock* ResultText;

	UPROPERTY(meta = (BindWidget))
	class UButton* QuitButton;

	UFUNCTION()
	void OnQuitClicked();
};
