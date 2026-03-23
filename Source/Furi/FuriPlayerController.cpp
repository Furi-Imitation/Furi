// Fill out your copyright notice in the Description page of Project Settings.


#include "FuriPlayerController.h"

#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

AFuriPlayerController::AFuriPlayerController()
{
	bShowMouseCursor = false;
}

void AFuriPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	TArray<AActor*> FoundCameras;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("MainCamera"), FoundCameras);
	
	if (FoundCameras.Num() > 0)
	{
		MainCameraActor = FoundCameras[0];
		// 이 컨트롤러의 시점을 해당 카메라로 고정합니다.
		SetViewTargetWithBlend(MainCameraActor);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MainCamera 태그가 붙은 카메라 액터를 찾을 수 없습니다."));
	}
}

void AFuriPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!MainCameraActor) return;
    
	// 1. 대상을 Character로 한정 (안정성)
	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), Players);
	if (Players.Num() == 0) return;
    
	FVector SumLocation = FVector::ZeroVector;
	for (AActor* Actor : Players) { SumLocation += Actor->GetActorLocation(); }
	FVector CenterTarget = SumLocation / Players.Num();
    
	float MaxDist = 0.f;
	if (Players.Num() >= 2)
	{
		MaxDist = FVector::Dist(Players[0]->GetActorLocation(), Players[1]->GetActorLocation());
	}
    
	FVector CurrentCamLoc = MainCameraActor->GetActorLocation();
       
	// 부드러운 줌 설계: 최소 높이를 더하고 시작
	float TargetZ = MinCameraHeight + (MaxDist * CameraPadding);
	TargetZ = FMath::Clamp(TargetZ, MinCameraHeight, MaxCameraHeight);
    
	// Z축 보간을 먼저 수행
	float NewZ = FMath::FInterpTo(CurrentCamLoc.Z, TargetZ, DeltaTime, ZoomInterpSpeed);

	// 비율 계산
	float HeightAlpha = FMath::GetMappedRangeValueClamped(
	   FVector2D(MinCameraHeight, MaxCameraHeight), 
	   FVector2D(0.f, 1.f), 
	   NewZ
	);

	// X 오프셋 계산 (Lerp 값은 에디터 수치에 맞게 조정)
	float DynamicOffsetX = FMath::Lerp(-400.f, CameraOffset.X, HeightAlpha);
    
	// 최종 위치 결정 (X, Y, Z를 하나의 보간 속도로 맞추는 것이 가장 깔끔함)
	FVector FinalTarget = CenterTarget + FVector(DynamicOffsetX, 0.f, 0.f);
	FinalTarget.Z = NewZ; // Z는 이미 위에서 계산됨

	// 최종 이동 (XY만 따라가기)
	FVector NextLoc;
	NextLoc.X = FMath::FInterpTo(CurrentCamLoc.X, FinalTarget.X, DeltaTime, CameraInterpSpeed);
	NextLoc.Y = FMath::FInterpTo(CurrentCamLoc.Y, FinalTarget.Y, DeltaTime, CameraInterpSpeed);
	NextLoc.Z = NewZ;

	MainCameraActor->SetActorLocation(NextLoc);
}