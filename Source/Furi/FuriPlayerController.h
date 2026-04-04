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
    
	void SetCinematicMode(bool bEnabled, AActor* TargetActor = nullptr);
	
	UFUNCTION(Client, Reliable)
	void Client_SetCinematicCamera(bool bEnabled, AActor* CameraSource);

	void UpdateStandardCamera(float DeltaTime);
	

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
    
private:
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
};
