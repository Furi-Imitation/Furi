#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReadyStartWidget.generated.h"

/**
 * 
 */
UCLASS()
class FURI_API UReadyStartWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 블루프린트 애니메이션이 끝났을 때 호출할 함수
	UFUNCTION(BlueprintCallable, Category = "UI")
	void NotifyAnimationFinished();

	// 블루프린트에서 애니메이션 재생 로직을 구현할 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void PlayReadyStartAnimation();
};
