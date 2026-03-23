// Fill out your copyright notice in the Description page of Project Settings.


#include "FuriPlayerController.h"

#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

AFuriPlayerController::AFuriPlayerController()
{
	bShowMouseCursor = false;
	bAutoManageActiveCameraTarget = false;
}

void AFuriPlayerController::BeginPlay()
{
	Super::BeginPlay();
    
	// 멀티플레이에서 클라이언트의 로컬 컨트롤러일 때만 카메라를 세팅하는 것이 안전합니다.
	if (IsLocalPlayerController())
	{
		TArray<AActor*> FoundCameras;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("MainCamera"), FoundCameras);
        
		if (FoundCameras.Num() > 0)
		{
			MainCameraActor = FoundCameras[0];
			SetViewTargetWithBlend(MainCameraActor);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("MainCamera 태그가 붙은 카메라 액터를 찾을 수 없습니다."));
		}
	}
}

void AFuriPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// 내 화면을 담당하는 로컬 컨트롤러가 아니면 연산을 멈춥니다.
	// 서버가 클라이언트의 카메라를 억지로 움직이려 하거나 연산이 두 번 겹쳐서
	// 카메라가 덜덜 떨리는(Jitter) 현상을 완벽하게 막아줍니다.
	if (!IsLocalPlayerController() || !MainCameraActor) return;
    
	// 1. 대상을 Character로 한정
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
       
	float TargetZ = MinCameraHeight + (MaxDist * CameraPadding);
	TargetZ = FMath::Clamp(TargetZ, MinCameraHeight, MaxCameraHeight);
    
	float NewZ = FMath::FInterpTo(CurrentCamLoc.Z, TargetZ, DeltaTime, ZoomInterpSpeed);

	float HeightAlpha = FMath::GetMappedRangeValueClamped(
	   FVector2D(MinCameraHeight, MaxCameraHeight), 
	   FVector2D(0.f, 1.f), 
	   NewZ
	);

	float DynamicOffsetX = FMath::Lerp(-400.f, CameraOffset.X, HeightAlpha);
    
	FVector FinalTarget = CenterTarget + FVector(DynamicOffsetX, 0.f, 0.f);
	FinalTarget.Z = NewZ;

	FVector NextLoc;
	NextLoc.X = FMath::FInterpTo(CurrentCamLoc.X, FinalTarget.X, DeltaTime, CameraInterpSpeed);
	NextLoc.Y = FMath::FInterpTo(CurrentCamLoc.Y, FinalTarget.Y, DeltaTime, CameraInterpSpeed);
	NextLoc.Z = NewZ;

	MainCameraActor->SetActorLocation(NextLoc);
}