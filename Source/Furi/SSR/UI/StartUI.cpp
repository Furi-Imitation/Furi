// Fill out your copyright notice in the Description page of Project Settings.


#include "StartUI.h"

#include "Components/Button.h"
#include "Furi/FuriPlayerController.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/UI/FuriGameHUDWidget.h"
#include "Furi/UI/FuriSkillIconWidget.h"
#include "Kismet/KismetSystemLibrary.h"

bool UStartUI::Initialize()
{
	bool Success = Super::Initialize();
	
	if (!Success)
		return false;
	
	if (StartButton)
	{
		StartButton->OnClicked.AddDynamic(this, &UStartUI::OnStartButtonClicked);
	}
	if (ExitButton)
	{
		ExitButton->OnClicked.AddDynamic(this, &UStartUI::OnQuitClicked);
	}
	
	return true;
}

void UStartUI::OnStartButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Start Button Clicked!"));
	
	// 1. 에셋 할당 체크 로그
	if (!FuriGameHUDWidgetClass || !FuriSkillIconWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("UI Classes are NULL! Check WBP_StartUI Blueprint!"));
		return;
	}
	
	if (FuriGameHUDWidgetClass && FuriSkillIconWidgetClass)
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		
		if (!PC) 
			return;
		
		UFuriGameHUDWidget* FuriGameHUDUI = CreateWidget<UFuriGameHUDWidget>(GetWorld(), FuriGameHUDWidgetClass);
		
		UFuriSkillIconWidget* FuriSkillIconUI = CreateWidget<UFuriSkillIconWidget>(GetWorld(), FuriSkillIconWidgetClass);
		
		
		if(FuriGameHUDUI) 
		{
			FuriGameHUDUI->AddToViewport();
			UE_LOG(LogTemp, Warning, TEXT("HUD Success"));
           
			if (AGasCharacterBase* MyChar = Cast<AGasCharacterBase>(PC->GetPawn()))
			{
				FuriGameHUDUI->InitPlayerStats(MyChar->GetAbilitySystemComponent());
				UE_LOG(LogTemp, Warning, TEXT("Character Stats Linked"));
			}
			else {
				UE_LOG(LogTemp, Error, TEXT("Pawn is NULL or Cast Failed!"));
			}
		}
       
		if(FuriSkillIconUI)
		{
			FuriSkillIconUI->AddToViewport();
			UE_LOG(LogTemp, Warning, TEXT("Skill UI Success"));
		}
		
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
		
		RemoveFromParent();
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
