#include "EndUI.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

bool UEndUI::Initialize()
{
	if (!Super::Initialize()) return false;

	if (RestartButton)
	{
		RestartButton->OnClicked.AddDynamic(this, &UEndUI::OnRestartClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UEndUI::OnQuitClicked);
	}

	return true;
}

void UEndUI::SetGameResult(bool bIsVictory)
{
	// 블루프린트에 선언된 이벤트를 호출합니다.
	OnGameResultDetermined(bIsVictory);
}


void UEndUI::OnRestartClicked()
{
	// 현재 레벨 재시작
	FString LevelName = GetWorld()->GetMapName();
	LevelName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	UGameplayStatics::OpenLevel(GetWorld(), FName(*LevelName));
}

void UEndUI::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, true);
}
