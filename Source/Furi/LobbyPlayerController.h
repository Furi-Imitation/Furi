#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

UCLASS()
class FURI_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override; // 🌟 추가됨!

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void RequestSetReady();

	UFUNCTION(Server, Reliable)
	void Server_SetReady();

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bIsReady = false;

protected:
	// 🌟 블루프린트에서 WBP_Start 위젯을 할당할 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UStartUI> StartUIClass;

private:
	UPROPERTY()
	class UStartUI* StartUIInstance;
};
