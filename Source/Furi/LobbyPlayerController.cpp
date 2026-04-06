#include "LobbyPlayerController.h"
#include "LobbyGameMode.h"
#include "Blueprint/UserWidget.h"
#include "SSR/UI/StartUI.h"

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 🌟 서버가 아닌 내 기기(로컬)일 때만 UI를 띄웁니다.
	if (IsLocalPlayerController())
	{
		if (StartUIClass)
		{
			StartUIInstance = CreateWidget<UStartUI>(this, StartUIClass);
			if (StartUIInstance)
			{
				StartUIInstance->AddToViewport();

				// 마우스를 켜고 UI만 클릭할 수 있게 모드 변경
				FInputModeUIOnly InputMode;
				InputMode.SetWidgetToFocus(StartUIInstance->TakeWidget());
				SetInputMode(InputMode);

				bShowMouseCursor = true;

				UE_LOG(LogTemp, Warning, TEXT("[Lobby] StartUI 화면에 생성 완료!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[Lobby] StartUIClass가 비어있습니다! 블루프린트를 확인하세요."));
		}
	}
}

void ALobbyPlayerController::RequestSetReady()
{
	if (bIsReady)
	{
		return;
	}

	bIsReady = true;
	Server_SetReady();
}

void ALobbyPlayerController::Server_SetReady_Implementation()
{
	if (ALobbyGameMode* GM = Cast<ALobbyGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->PlayerReady();
	}
}
