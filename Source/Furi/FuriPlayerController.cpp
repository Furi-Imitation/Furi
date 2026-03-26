#include "FuriPlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "GamePlayAbilitySystem/Characters/GasCharacterBase.h"
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
            DefaultMainCameraRotation = MainCameraActor->GetActorRotation();
        }
    }
}

void AFuriPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (!IsLocalPlayerController() || !MainCameraActor) return;

    // 🌟 연출 모드일 때는 외부 카메라 로직 중단
    if (bIsCinematicMode) return;

    UpdateStandardCamera(DeltaTime);
}

void AFuriPlayerController::UpdateStandardCamera(float DeltaTime)
{
    TArray<AActor*> Players;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), Players);
    if (Players.Num() == 0) return;
    
    FVector SumLocation = FVector::ZeroVector;
    for (AActor* Actor : Players) { SumLocation += Actor->GetActorLocation(); }
    FVector CenterTarget = SumLocation / Players.Num();
    
    float MaxDist = (Players.Num() >= 2) ? FVector::Dist(Players[0]->GetActorLocation(), Players[1]->GetActorLocation()) : 0.f;
    
    FVector CurrentCamLoc = MainCameraActor->GetActorLocation();
    float TargetZ = FMath::Clamp(MinCameraHeight + (MaxDist * CameraPadding), MinCameraHeight, MaxCameraHeight);
    float NewZ = FMath::FInterpTo(CurrentCamLoc.Z, TargetZ, DeltaTime, ZoomInterpSpeed);

    float HeightAlpha = FMath::GetMappedRangeValueClamped(FVector2D(MinCameraHeight, MaxCameraHeight), FVector2D(0.f, 1.f), NewZ);
    float DynamicOffsetX = FMath::Lerp(-400.f, CameraOffset.X, HeightAlpha);
    
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

    AGasCharacterBase* MyChar = Cast<AGasCharacterBase>(GetPawn());
    if (!MyChar) return;

    if (bEnabled)
    {
        // 🌟 1. 캐릭터 안에 있는 'UltCamera' 시점으로 전환
        SetViewTargetWithBlend(MyChar, 0.2f, EViewTargetBlendFunction::VTBlend_Cubic);

        // 🌟 2. SpringArm을 옆면 구도로 조정
        if (USpringArmComponent* Arm = MyChar->GetUltSpringArm())
        {
            Arm->TargetArmLength = 350.0f;
            Arm->SocketOffset = FVector(0.0f, 150.0f, 60.0f);
            
            if (TargetActor)
            {
                // 적과 나 사이의 방향을 계산해 측면 구도 잡기
                FVector Dir = TargetActor->GetActorLocation() - MyChar->GetActorLocation();
                Arm->SetWorldRotation(Dir.Rotation() + FRotator(0, -90, 0));
            }
        }
    }
    else
    {
        // 🌟 3. 원래 외부 메인 카메라로 복구
        if (MainCameraActor)
        {
            SetViewTargetWithBlend(MainCameraActor, 0.3f, EViewTargetBlendFunction::VTBlend_Cubic);
            MainCameraActor->SetActorRotation(DefaultMainCameraRotation);
        }
    }
}