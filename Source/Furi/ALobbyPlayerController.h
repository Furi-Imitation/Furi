#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ALobbyPlayerController.generated.h"

UCLASS()
class FURI_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// UI에서 버튼을 눌렀을 때 호출할 함수 (블루프린트에서 부르기 위해 Callable 설정)
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void RequestSetReady();

	// 🌟 서버에게 내가 준비되었다고 알리는 RPC 함수
	UFUNCTION(Server, Reliable)
	void Server_SetReady();

	// 내가 현재 준비 상태인지 저장 (UI 업데이트용)
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bIsReady = false;
};