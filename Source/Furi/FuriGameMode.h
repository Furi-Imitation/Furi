// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FuriGameMode.generated.h"

/**
 * 
 */
UCLASS()
class FURI_API AFuriGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// 컨트롤러에 따라 어떤 폰(Pawn) 클래스를 줄지 결정하는 함수입니다.
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	AFuriGameMode();

protected:
	// 플레이어 스폰 위치 정해주는 함수
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	// 에디터에서 선택할 수 있게 노출
	UPROPERTY(EditAnywhere, Category = "Classes")
	TSubclassOf<APawn> HostClass; // BP_P1Player 담을 곳

	UPROPERTY(EditAnywhere, Category = "Classes")
	TSubclassOf<APawn> ClientClass; // BP_SSRPlayer 담을 곳

	virtual void BeginPlay() override; // 게임 시작 시 호출될 함수

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> StartUIClass; // 에디터에서 StartUI 블루프린트 선택

	UPROPERTY()
	class UUserWidget* StartUIInstance; // 생성된 위젯 저장용

private:
	int32 SpawnedPlayerCount = 0;
};
