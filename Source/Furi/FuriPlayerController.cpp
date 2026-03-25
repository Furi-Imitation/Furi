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
    
    if (IsLocalPlayerController())
    {
        TArray<AActor*> FoundCameras;
        UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("MainCamera"), FoundCameras);
        
        if (FoundCameras.Num() > 0)
        {
            MainCameraActor = FoundCameras[0];
            SetViewTargetWithBlend(MainCameraActor);
            
            // 🌟 시작할 때 카메라의 기본 회전값(수직 아래 등)을 저장해둡니다.
            DefaultCameraRotation = MainCameraActor->GetActorRotation();
        }
    }
}

void AFuriPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    if (!IsLocalPlayerController() || !MainCameraActor) return;

    // 1. 연출 모드 로직
    if (bIsCinematicMode && CinematicTarget)
    {
        FVector TargetLoc = CinematicTarget->GetActorLocation();
        FVector SideViewLoc = TargetLoc + (CinematicTarget->GetActorRightVector() * 300.f) + (FVector::UpVector * 200.f);
        
        FVector CurrentLoc = MainCameraActor->GetActorLocation();
        FVector NewLoc = FMath::VInterpTo(CurrentLoc, SideViewLoc, DeltaTime, 5.0f);
        MainCameraActor->SetActorLocation(NewLoc);

        FRotator TargetRot = FRotationMatrix::MakeFromX(TargetLoc - NewLoc).Rotator();
        MainCameraActor->SetActorRotation(FMath::RInterpTo(MainCameraActor->GetActorRotation(), TargetRot, DeltaTime, 5.0f));
    }
    // 2. 일반 추적 모드 로직 (함수로 분리됨)
    else
    {
        UpdateStandardCamera(DeltaTime);
    }
}

void AFuriPlayerController::UpdateStandardCamera(float DeltaTime)
{
    TArray<AActor*> Players;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), Players);
    if (Players.Num() == 0) return;
    
    // 평균 위치 및 최대 거리 계산
    FVector SumLocation = FVector::ZeroVector;
    for (AActor* Actor : Players) { SumLocation += Actor->GetActorLocation(); }
    FVector CenterTarget = SumLocation / Players.Num();
    
    float MaxDist = 0.f;
    if (Players.Num() >= 2)
    {
       MaxDist = FVector::Dist(Players[0]->GetActorLocation(), Players[1]->GetActorLocation());
    }
    
    FVector CurrentCamLoc = MainCameraActor->GetActorLocation();
       
    // Z값(높이) 및 줌 계산
    float TargetZ = MinCameraHeight + (MaxDist * CameraPadding);
    TargetZ = FMath::Clamp(TargetZ, MinCameraHeight, MaxCameraHeight);
    float NewZ = FMath::FInterpTo(CurrentCamLoc.Z, TargetZ, DeltaTime, ZoomInterpSpeed);

    float HeightAlpha = FMath::GetMappedRangeValueClamped(FVector2D(MinCameraHeight, MaxCameraHeight), FVector2D(0.f, 1.f), NewZ);
    float DynamicOffsetX = FMath::Lerp(-400.f, CameraOffset.X, HeightAlpha);
    
    // 최종 위치 계산 및 적용
    FVector FinalTarget = CenterTarget + FVector(DynamicOffsetX, 0.f, 0.f);
    FinalTarget.Z = NewZ;

    FVector NextLoc;
    NextLoc.X = FMath::FInterpTo(CurrentCamLoc.X, FinalTarget.X, DeltaTime, CameraInterpSpeed);
    NextLoc.Y = FMath::FInterpTo(CurrentCamLoc.Y, FinalTarget.Y, DeltaTime, CameraInterpSpeed);
    NextLoc.Z = NewZ;

    MainCameraActor->SetActorLocation(NextLoc);
}

void AFuriPlayerController::SetCinematicMode(bool bEnabled, AActor* TargetActor)
{
    bIsCinematicMode = bEnabled;
    CinematicTarget = TargetActor;
    
    if (!bEnabled && MainCameraActor)
    {
        // 🌟 연출이 종료될 때 카메라 회전을 원래 저장해둔 기본값으로 복구합니다.
        // Interp를 쓰지 않고 즉시 돌려놓으면 UpdateStandardCamera가 위치를 잡을 때 깔끔합니다.
        // 만약 회전도 부드럽게 돌아가길 원하면 RInterp를 Tick에서 bIsCinematicMode가 아닐 때 수행하면 됩니다.
        MainCameraActor->SetActorRotation(DefaultCameraRotation);
        
        UE_LOG(LogTemp, Log, TEXT("Cinematic Mode OFF: Camera Rotation Restored."));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Cinematic Mode ON: Target is %s"), TargetActor ? *TargetActor->GetName() : TEXT("None"));
    }
}