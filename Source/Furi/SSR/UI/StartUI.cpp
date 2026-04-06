// Fill out your copyright notice in the Description page of Project Settings.


#include "StartUI.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Furi/FuriPlayerController.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/UI/FuriGameHUDWidget.h"
#include "Furi/UI/FuriSkillIconWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

bool UStartUI::Initialize()
{
	bool Success = Super::Initialize();
	
	if (!Success)
		return false;
	
	// 초기 상태 설정
	if (WaitingText)
	{
		WaitingText->SetVisibility(ESlateVisibility::Hidden);
	}
	
	if (ReadyButton)
	{
		ReadyButton->OnClicked.AddDynamic(this, &UStartUI::OnStartButtonClicked);
	}
	if (ExitButton)
	{
		ExitButton->OnClicked.AddDynamic(this, &UStartUI::OnQuitClicked);
	}
	
	return true;
}

void UStartUI::OnStartButtonClicked()
{
	// UE_LOG(LogTemp, Warning, TEXT("Start Button Clicked!"));
	//
	// APlayerController* PC = GetWorld()->GetFirstPlayerController();
	// 	
	// if (PC)
	// {
	// 	FInputModeGameOnly InputMode;
	// 	PC->SetInputMode(InputMode);
	// 	PC->bShowMouseCursor = false;
	// }	
	//
	// // 리슨 서버로 level을 연다
	// UGameplayStatics::OpenLevel(GetWorld(), FName("Lvl_Furi"), true, TEXT("Listen"));
	// RemoveFromParent();
	// -------------------------------------------------
	
	// 로컬 플레이어 컨트롤러 가져오기
	AFuriPlayerController* PC = Cast<AFuriPlayerController>(GetOwningPlayer());
    
	if (PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("Local PlayerController Found! Sending RPC..."));
		PC->Server_SetReady(true); // 이 요청이 서버로 가야 합니다.
		UpdateReadyVisual(true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerController NOT Found in StartUI!"));
	}
}

void UStartUI::UpdateReadyVisual(bool bIsReady)
{
	if (bIsReady)
	{
		if (ReadyButton) ReadyButton->SetIsEnabled(false); // 다시 못 누르게 비활성화
		ShowWaitingMessage(true);
	}
}

void UStartUI::ShowWaitingMessage(bool bShow)
{
	if (WaitingText)
	{
		WaitingText->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		WaitingText->SetText(FText::FromString(TEXT("Waiting for other player...")));
	}
}


void UStartUI::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this,
		GetWorld()->GetFirstPlayerController(),
		EQuitPreference::Quit, 
		true
		);
}
