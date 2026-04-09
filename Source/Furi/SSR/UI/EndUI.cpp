#include "EndUI.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

bool UEndUI::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
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


void UEndUI::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, true);
}
