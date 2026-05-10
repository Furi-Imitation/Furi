#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FuriDebugWidget.generated.h"

class UFuriDebugComponent;

UCLASS()
class FURI_API UFuriDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Furi|Debug")
	void SetDebugComponent(UFuriDebugComponent* InDebugComp) { DebugComponent = InDebugComp; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Furi|Debug")
	UFuriDebugComponent* DebugComponent;
};
