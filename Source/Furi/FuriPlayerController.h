// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FuriPlayerController.generated.h"

class UFuriGameHUDWidget;
/**
 * 
 */
UCLASS()
class FURI_API AFuriPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFuriPlayerController();

	void SetFuriCinematicMode(bool bEnabled, AActor* TargetActor = nullptr);

	UFUNCTION(Client, Reliable)
	void Client_SetCinematicCamera(bool bEnabled, AActor* CameraSource);

	void UpdateStandardCamera(float DeltaTime);

	void Client_ShowFightUI();

	UFUNCTION(Client, Reliable)
	void Client_ShowReadyStartUI();

	UFUNCTION(Server, Reliable)
	void Server_NotifyReadyToFight();

	UFUNCTION(Client, Reliable)
	void Client_StartGameHUDTimer();

	// 🌟 카메라 페이드를 제어하기 위한 RPC
	UFUNCTION(Client, Reliable)
	void Client_FadeCamera(bool bFadeOut, float Duration);

	// 🌟 클라이언트에서 캐릭터 및 시네마틱 요소를 숨기기 위한 RPC
	UFUNCTION(Client, Reliable)
	void Client_SetActorHidden(AActor* TargetActor, bool bShouldHide);

	UFUNCTION(Client, Reliable)
	void Client_SetCinematicActorsHidden(bool bShouldHide);

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UReadyStartWidget> ReadyStartWidgetClass;

	UPROPERTY()
	class UReadyStartWidget* ReadyStartWidgetInstance;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraInterpSpeed = 7.0f;
	UPROPERTY(EditAnywhere, Category = "Camera")
	float ZoomInterpSpeed = 5.0f;
	UPROPERTY(EditAnywhere, Category = "Camera")
	float MinCameraHeight = 400.0f;
	UPROPERTY(EditAnywhere, Category = "Camera")
	float MaxCameraHeight = 3000.0f;
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraPadding = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector CameraOffset = FVector(-3000.f, 0.f, 0.f);

	UPROPERTY()
	AActor* MainCameraActor;

	bool bIsCinematicMode = false;
	UPROPERTY()
	AActor* CinematicTarget = nullptr;

	// 복구용 기본값
	FRotator DefaultMainCameraRotation;

protected:
	//HUD 생성
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UFuriGameHUDWidget> MainHUDWidgetClass;

	// 🌟 실제로 화면에 띄워진 위젯의 메모리를 쥐고 있을 포인터
	UPROPERTY(Transient)
	TObjectPtr<UFuriGameHUDWidget> MainHUDWidget;

	// 🌟 적 탐색을 위한 타이머 핸들
	FTimerHandle EnemySearchTimerHandle;

	// 🌟 적을 찾아서 UI에 연결 시도하는 함수
	void TryFindEnemyForHUD();


	// 에디터(BP_FuriPlayerController)에서 WBP_EndUI를 선택할 변수
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UEndUI> EndUIClass;

	// 생성된 UI 인스턴스를 보관할 포인터
	UPROPERTY()
	class UEndUI* EndUIInstance;

public:
	// 외부(캐릭터 사망 등)에서 호출할 게임 종료 함수
	void ShowGameEndUI(bool bVictory);

	// NetMulticast나 Client 키워드를 사용하여 서버가 클라이언트에게 명령하게 합니다.
	UFUNCTION(Client, Reliable)
	void Client_ShowGameEndUI(bool bVictory);
	
	// EndUI나오기전에 사망 + 승리 애니메이션 출력
public:
	// 특정 타겟을 집중 조명하고 애니메이션을 재생하는 함수
	UFUNCTION(Client, Reliable)
	void Client_PlayFinishingSequence(AActor* TargetActor, bool bIsWinner);

	// EndUI만 따로 띄우는 함수 (기존 ShowGameEndUI를 활용하거나 분리)
	UFUNCTION(Client, Reliable)
	void Client_FinalShowUI(bool bVictory);

protected:
	// 카메라 보간용 변수
	bool bIsFinishingFocus = false;
	UPROPERTY()
	AActor* FinishFocusTarget = nullptr;
    
	// PlayerTick에서 카메라를 부드럽게 옮기기 위해 로직 수정이 필요할 수 있음
};
