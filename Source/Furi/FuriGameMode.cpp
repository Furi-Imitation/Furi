// Fill out your copyright notice in the Description page of Project Settings.


#include "FuriGameMode.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

AFuriGameMode::AFuriGameMode()
{
	SpawnedPlayerCount = 0;
}

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

AActor* AFuriGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// 1. 맵에 있는 모든 PlayerStart 액터를 찾아냅니다.
	TArray<AActor*> FoundPlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundPlayerStarts);

	// 2. 현재 몇 명째 스폰인지에 따라 찾을 태그(P1 또는 P2)를 결정합니다.
	FName TargetTag = (SpawnedPlayerCount == 0) ? FName("P1") : FName("P2");

	// 3. 찾아낸 PlayerStart들 중에서 태그가 일치하는 곳을 선택합니다.
	for (AActor* StartActor : FoundPlayerStarts)
	{
		APlayerStart* PStart = Cast<APlayerStart>(StartActor);
		if (PStart && PStart->PlayerStartTag == TargetTag)
		{
			// 태그가 일치하면 카운트를 올리고 해당 위치를 스폰 지점으로 반환합니다.
			SpawnedPlayerCount++;

			UE_LOG(LogTemp, Warning, TEXT("[GameMode] 플레이어 스폰 위치 확정: %s"), *TargetTag.ToString());
			return PStart;
		}
	}

	// 만약 P1, P2 태그를 못 찾았다면 엔진의 기본 스폰 로직을 따릅니다. (안전장치)
	UE_LOG(LogTemp, Error, TEXT("[GameMode] P1, P2 태그가 설정된 PlayerStart를 찾지 못했습니다!"));
	return Super::ChoosePlayerStart_Implementation(Player);
}
