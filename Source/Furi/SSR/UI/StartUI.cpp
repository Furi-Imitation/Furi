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
	if (FuriGameHUDWidgetClass && FuriSkillIconWidgetClass)
	{
		UFuriGameHUDWidget* FuriGameHUDUI = CreateWidget<UFuriGameHUDWidget>(GetWorld(), FuriGameHUDWidgetClass);
		
		UFuriSkillIconWidget* FuriSkillIconUI = CreateWidget<UFuriSkillIconWidget>(GetWorld(), FuriSkillIconWidgetClass);
		
		
		if(FuriGameHUDUI) 
		{
			FuriGameHUDUI->AddToViewport();
           
			// 2. 캐릭터 데이터 연동 (중요!)
			if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(GetOwningPlayer()))
			{
				if (AGasCharacterBase* MyChar = Cast<AGasCharacterBase>(PC->GetPawn()))
				{
					FuriGameHUDUI->InitPlayerStats(MyChar->GetAbilitySystemComponent());
				}
               
				// 적 탐색 타이머도 여기서 다시 시작하거나, PC에 있는 함수를 호출해줘야 합니다.
				// PC->TryFindEnemyForHUD(); // PC에 이 함수가 public으로 있다면 호출 가능
			}
		}
       
		if(FuriSkillIconUI) FuriSkillIconUI->AddToViewport();

		// 3. 게임 모드 전환 및 자신 삭제
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (PC)
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;
		}
		
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
