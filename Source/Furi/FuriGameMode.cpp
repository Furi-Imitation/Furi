// Fill out your copyright notice in the Description page of Project Settings.


#include "FuriGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"


void AFuriGameMode::BeginPlay()
{
	Super::BeginPlay();
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
