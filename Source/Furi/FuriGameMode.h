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
	
	void CheckAllPlayersReady();

protected:
	// 에디터에서 선택할 수 있게 노출
	UPROPERTY(EditAnywhere, Category = "Classes")
	TSubclassOf<APawn> HostClass; // BP_P1Player 담을 곳

	UPROPERTY(EditAnywhere, Category = "Classes")
	TSubclassOf<APawn> ClientClass; // BP_SSRPlayer 담을 곳
	
	virtual void BeginPlay() override; // 게임 시작 시 호출될 함수
	
	
	// --- [기존 StartGameMode에서 가져온 UI 로직] ---
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> StartUIClass;
	
	UPROPERTY()
	UUserWidget* StartUIInstance;

	// 블루프린트 디테일창에서 HUD 위젯 클래스를 선택할 수 있게 합니다.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UFuriGameHUDWidget> FuriGameHUDWidgetClass;

	// 생성된 HUD 인스턴스를 보관하고 싶다면 선언 (선택 사항)
	UPROPERTY()
	UFuriGameHUDWidget* CurrentHUD;
	
	
};
