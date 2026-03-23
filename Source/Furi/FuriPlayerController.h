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
	
protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	
private:
	// 카메라가 타겟을 따라가는 부드러움 (높을수록 빠름)
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraInterpSpeed = 7.0f;

	// 줌 속도
	UPROPERTY(EditAnywhere, Category = "Camera")
	float ZoomInterpSpeed = 5.0f;
	
	// 최소/최대 줌 거리	
	UPROPERTY(EditAnywhere, Category = "Camera")
	float MinCameraHeight = 400.0f;
	
	UPROPERTY(EditAnywhere, Category = "Camera")
	float MaxCameraHeight = 3000.0f;
	
	// 여유 계수(Padding Factor) 적용
	// 수치가 높을수록 카메라가 더 높이 올라가서 캐릭터 주변 여백이 넓어집니다.
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraPadding = 1.0f;
	
	// 카메라가 캐릭터 뒤쪽으로 얼마나 물러날지 결정하는 오프셋
	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector CameraOffset = FVector(-3000.f, 0.f, 0.f);
private:
	// 월드에서 찾은 메인 카메라 액터 보관
	UPROPERTY()
	AActor* MainCameraActor;
};
