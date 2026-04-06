// Fill out your copyright notice in the Description page of Project Settings.


#include "FuriGameMode.h"

#include "FuriPlayerController.h"
#include "UI/FuriGameHUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"


void AFuriGameMode::BeginPlay()
{
	// Super::BeginPlay();
	//
	// // 1. 위젯 생성 및 뷰포트 추가
	// if (StartUIClass)
	// {
	// 	StartUIInstance = CreateWidget<UUserWidget>(GetWorld(), StartUIClass);
	// 	if (StartUIInstance)
	// 	{
	// 		StartUIInstance->AddToViewport();
	//
	// 		// 2. 플레이어 컨트롤러 설정 (마우스 및 입력 모드)
	// 		APlayerController* PC = GetWorld()->GetFirstPlayerController();
	// 		if (PC)
	// 		{
	// 			// UI에 포커스를 주고 마우스 커서를 보이게 함
	// 			FInputModeUIOnly InputMode;
	// 			InputMode.SetWidgetToFocus(StartUIInstance->TakeWidget());
	// 			PC->SetInputMode(InputMode);
	// 			PC->bShowMouseCursor = true;
	// 		}
	// 	}
	// }
	
	Super::BeginPlay();

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	// 1. 현재 맵 이름을 소문자로 변환하여 정확히 체크
	FString MapName = GetWorld()->GetMapName().ToLower();

	// --- [A. 로비(StartMap)인 경우] ---
	if (MapName.Contains(TEXT("startmap"))) 
	{
		// 인게임 HUD가 혹시 떠 있다면 제거 (안전장치)
		if (CurrentHUD) CurrentHUD->RemoveFromParent();

		if (StartUIClass)
		{
			StartUIInstance = CreateWidget<UUserWidget>(GetWorld(), StartUIClass);
			if (StartUIInstance)
			{
				StartUIInstance->AddToViewport();
                
				// 마우스 커서 활성화 및 UI 포커스
				FInputModeUIOnly InputMode;
				InputMode.SetWidgetToFocus(StartUIInstance->TakeWidget());
				PC->SetInputMode(InputMode);
				PC->bShowMouseCursor = true;
                
				UE_LOG(LogTemp, Warning, TEXT("Lobby UI Created on: %s"), *MapName);
				return; // 로비 로직 완료 후 종료 (아래 인게임 HUD 로직 실행 방지)
			}
		}
	}
    
	// --- [B. 인게임(Lvl_Furi)인 경우] ---
	// 로비가 아니거나 맵 이름에 furi가 포함된 경우
	if (MapName.Contains(TEXT("furi"))) 
	{
		if (FuriGameHUDWidgetClass)
		{
			CurrentHUD = CreateWidget<UFuriGameHUDWidget>(PC, FuriGameHUDWidgetClass);
			if (CurrentHUD)
			{
				CurrentHUD->AddToViewport();

				// 게임 모드로 전환 (마우스 숨기기)
				FInputModeGameOnly InputMode;
				PC->SetInputMode(InputMode);
				PC->bShowMouseCursor = false;
                
				UE_LOG(LogTemp, Warning, TEXT("In-Game HUD Created on: %s"), *MapName);
			}
		}
	}
	
}

UClass* AFuriGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// // 1. 호스트(서버 리스너)인지 확인
	// // 로컬 플레이어 컨트롤러이면서 서버 권한이 있다면 보통 호스트입니다.
	// if (InController->IsLocalController())
	// {
	// 	return HostClass;
	// }
	//
	// // 2. 그 외 접속자(클라이언트)에게는 다른 클래스를 반환
	// return ClientClass;
	
	// --------------------------------------
	
	// 1. 서버의 로컬 컨트롤러(방장)인지 확인
	if (InController->IsLocalPlayerController() && InController->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawning Host Class"));
		return HostClass;
	}

	// 2. 그 외(멀티플레이로 접속한 클라이언트)
	UE_LOG(LogTemp, Warning, TEXT("Spawning Client Class"));
	return ClientClass;
}

void AFuriGameMode::CheckAllPlayersReady()
{
	int32 ReadyCount = 0;
	int32 PlayerCount = 0;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFuriPlayerController* PC = Cast<AFuriPlayerController>(It->Get());
		if (PC)
		{
			PlayerCount++;
			if (PC->IsReady()) 
				ReadyCount++; // PC에 IsReady() 함수 구현 필요
		}
	}
	
	// 로그로 확인 (디버깅용)
	UE_LOG(LogTemp, Warning, TEXT("Players: %d, Ready: %d"), PlayerCount, ReadyCount);

	// 2명이 접속했고 둘 다 준비되었다면 맵 이동
	if (PlayerCount >= 2 && ReadyCount >= PlayerCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("All Players Ready! Traveling..."));
		GetWorld()->ServerTravel(TEXT("/Maps/Lvl_Furi"));
	}
}
