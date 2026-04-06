#include "LobbyGameMode.h"

ALobbyGameMode::ALobbyGameMode()
{
	// 언리얼 멀티플레이어 맵 이동의 정석: 끊김 없는 이동을 위해 SeamlessTravel 활성화
	bUseSeamlessTravel = true;
}

void ALobbyGameMode::PlayerReady()
{
	ReadyPlayerCount++;

	// 🌟 서버 로그에 현재 상태를 명확히 찍어봅니다.
	UE_LOG(LogTemp, Warning, TEXT("==== [Lobby] Player Ready Called! Current: %d / Target: %d ===="),
	       ReadyPlayerCount, RequiredPlayersToStart);

	if (ReadyPlayerCount >= RequiredPlayersToStart)
	{
		// 🌟 맵 경로가 틀리면 여기서 실패합니다. 실제 폴더 경로를 확인하세요!
		// 예: /Game/Maps/MainMap -> Content/Maps/MainMap.umap
		FString MapPath = TEXT("/Game/Maps/Lvl_Furi?listen");

		UE_LOG(LogTemp, Error, TEXT("==== [Lobby] All Players Ready! Traveling to: %s ===="), *MapPath);

		GetWorld()->ServerTravel(MapPath);
	}
}
