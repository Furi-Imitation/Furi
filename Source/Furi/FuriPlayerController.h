// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FuriPlayerController.generated.h"

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
};
