// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FuriGameMode.generated.h"

class ULevelSequence;
class ULevelSequencePlayer;
class AGasCharacterBase;
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

	// 블루프린트에서 설정할 시퀀스 에셋
	UPROPERTY(EditDefaultsOnly, Category = "Furi|Cinematic")
	ULevelSequence* IntroSequenceAsset;

private:
	FTimerHandle CheckPlayersTimerHandle;
	void CheckAllPlayersReady();

	void StartIntroSequence();

	// 🌟 시퀀스 재생이 완료되면 자동으로 호출될 함수
	UFUNCTION()
	void EndIntroAndStartFight();

	// 시퀀스 플레이어 관리용 변수
	UPROPERTY()
	ULevelSequencePlayer* IntroSequencePlayer;

	int32 SpawnedPlayerCount = 0;
	
public:
	void ProcessMatchEnd(AGasCharacterBase* Winner, AGasCharacterBase* Loser);
	
	void OnDeathAnimationFinished();
	void OnVictoryAnimationFinished();
	
private:
	UPROPERTY()
	AGasCharacterBase* CachedWinner;
	UPROPERTY()
	AGasCharacterBase* CachedLoser;
};
