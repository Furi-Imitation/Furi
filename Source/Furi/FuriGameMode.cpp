// Fill out your copyright notice in the Description page of Project Settings.


#include "FuriGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"


void AFuriGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 1. 위젯 생성 및 뷰포트 추가
	if (StartUIClass)
	{
		StartUIInstance = CreateWidget<UUserWidget>(GetWorld(), StartUIClass);
		if (StartUIInstance)
		{
			StartUIInstance->AddToViewport();

			// 2. 플레이어 컨트롤러 설정 (마우스 및 입력 모드)
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (PC)
			{
				// UI에 포커스를 주고 마우스 커서를 보이게 함
				FInputModeUIOnly InputMode;
				InputMode.SetWidgetToFocus(StartUIInstance->TakeWidget());
				PC->SetInputMode(InputMode);
				PC->bShowMouseCursor = true;
			}
		}
	}
}

UClass* AFuriGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// 1. 호스트(서버 리스너)인지 확인
	// 로컬 플레이어 컨트롤러이면서 서버 권한이 있다면 보통 호스트입니다.
	if (InController->IsLocalController())
	{
		return HostClass;
	}

	// 2. 그 외 접속자(클라이언트)에게는 다른 클래스를 반환
	return ClientClass;
}
