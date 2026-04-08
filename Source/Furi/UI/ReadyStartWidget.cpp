#include "ReadyStartWidget.h"
#include "Furi/FuriPlayerController.h"

void UReadyStartWidget::NotifyAnimationFinished()
{
	if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(GetOwningPlayer()))
	{
		PC->Server_NotifyReadyToFight();
	}
	
	RemoveFromParent();
}
