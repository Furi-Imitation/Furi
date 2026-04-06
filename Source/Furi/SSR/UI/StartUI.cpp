// Fill out your copyright notice in the Description page of Project Settings.

#include "StartUI.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h"

// 🌟 이전 답변에서 만든 로비 전용 컨트롤러 헤더를 인클루드 합니다. (경로는 프로젝트에 맞게 수정하세요)
#include "Furi/LobbyPlayerController.h"

bool UStartUI::Initialize()
{
	bool Success = Super::Initialize();

	if (!Success)
	{
		return false;
	}

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
	UE_LOG(LogTemp, Warning, TEXT("[StartUI] 준비 완료 버튼 클릭!"));

	// 1. 내 UI를 소유하고 있는 플레이어 컨트롤러를 가져옵니다.
	// 🌟 GetFirstPlayerController() 대신 GetOwningPlayer()를 써야 멀티플레이에서 안전합니다.
	if (ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(GetOwningPlayer()))
	{
		// 2. 컨트롤러에게 서버로 '준비 완료' RPC 신호를 보내라고 명령합니다.
		LobbyPC->RequestSetReady();

		// 3. UI 시각적 업데이트 (중복 클릭 방지)
		StartButton->SetIsEnabled(false);

		if (StartButtonText)
		{
			StartButtonText->SetText(FText::FromString(TEXT("Waiting for Other Player...")));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[StartUI] 현재 컨트롤러가 ALobbyPlayerController가 아닙니다!"));
	}
}

void UStartUI::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(
		this,
		GetOwningPlayer(), // 여기도 GetOwningPlayer()로 수정
		EQuitPreference::Quit,
		true
	);
}
