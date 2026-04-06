#include "StartGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

void AFuriStartGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	if (StartUIClass)
	{
		StartUIInstance = CreateWidget<UUserWidget>(GetWorld(), StartUIClass);
		if (StartUIInstance)
		{
			StartUIInstance->AddToViewport();
			
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (PC)
			{
				FInputModeUIOnly InputMode;
				InputMode.SetWidgetToFocus(StartUIInstance->TakeWidget());
				PC->SetInputMode(InputMode);
				PC->bShowMouseCursor = true;
				
				// 시작화면에는 캐릭터가 필요없다
				DefaultPawnClass = nullptr;
			}
		}
	}
}
