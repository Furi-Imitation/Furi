#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGameMode.generated.h"

UCLASS()
class FURI_API ALobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALobbyGameMode();

	// 플레이어가 준비 완료를 누를 때마다 컨트롤러가 호출해 줄 함수
	void PlayerReady();

protected:
	// 현재 준비된 플레이어 수
	int32 ReadyPlayerCount = 0;

	// 게임을 시작하기 위해 필요한 총 플레이어 수 (1vs1 이면 2)
	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	int32 RequiredPlayersToStart = 2;
};
