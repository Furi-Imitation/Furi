#include "FuriPlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Kismet/GameplayStatics.h"

AFuriPlayerController::AFuriPlayerController()
{
    // 게임 플레이 중 마우스 커서를 숨깁니다. (액션 게임 특성)
    bShowMouseCursor = false;
    // 언리얼 엔진이 뷰 타겟을 자동으로 변경하는 것을 막고, 우리가 직접 제어합니다.
    bAutoManageActiveCameraTarget = false; 
}

void AFuriPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // 현재 기기를 조작하는 로컬 플레이어인지 확인합니다. (멀티플레이어 환경 대비)
    if (IsLocalPlayerController())
    {
        // 맵에 배치된 외부 카메라 중 'MainCamera' 태그를 가진 액터를 찾습니다.
        TArray<AActor*> FoundCameras;
        UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("MainCamera"), FoundCameras);
        
        if (FoundCameras.Num() > 0)
        {
            MainCameraActor = FoundCameras[0];
            
            // 게임이 시작되면 플레이어의 시선을 이 메인 카메라로 즉시 설정합니다.
            SetViewTargetWithBlend(MainCameraActor);
            
            // 시네마틱 연출 후 원래 구도로 복구하기 위해 초기 회전값을 저장해 둡니다.
            DefaultMainCameraRotation = MainCameraActor->GetActorRotation();
        }
    }
}

void AFuriPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (!IsLocalPlayerController() || !MainCameraActor) return;

    // 🌟 궁극기 등 시네마틱 연출 모드일 때는 카메라가 플레이어들을 쫓아다니지 않도록 막습니다.
    if (bIsCinematicMode) return;

    // 일반 전투 상황이라면 두 플레이어를 화면에 담기 위해 카메라 위치를 업데이트합니다.
    UpdateStandardCamera(DeltaTime);
}

void AFuriPlayerController::UpdateStandardCamera(float DeltaTime)
{
    // 맵에 있는 모든 캐릭터(플레이어와 적)를 가져옵니다.
    TArray<AActor*> Players;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), Players);
    if (Players.Num() == 0) return;
    
    // 1. 모든 캐릭터의 위치를 더해 중간 지점(CenterTarget)을 계산합니다.
    FVector SumLocation = FVector::ZeroVector;
    for (AActor* Actor : Players) { SumLocation += Actor->GetActorLocation(); }
    FVector CenterTarget = SumLocation / Players.Num();
    
    // 2. 캐릭터들 사이의 가장 먼 거리를 구합니다. (1vs1 기준)
    float MaxDist = (Players.Num() >= 2) ? FVector::Dist(Players[0]->GetActorLocation(), Players[1]->GetActorLocation()) : 0.f;
    
    FVector CurrentCamLoc = MainCameraActor->GetActorLocation();
    
    // 3. 거리에 비례하여 카메라의 목표 높이(TargetZ)를 계산하고, 최소/최대 높이로 제한(Clamp)합니다.
    float TargetZ = FMath::Clamp(MinCameraHeight + (MaxDist * CameraPadding), MinCameraHeight, MaxCameraHeight);
    
    // 현재 높이에서 목표 높이로 부드럽게 이동(Interpolation)합니다.
    float NewZ = FMath::FInterpTo(CurrentCamLoc.Z, TargetZ, DeltaTime, ZoomInterpSpeed);

    // 4. 높이에 따라 X축 오프셋을 동적으로 조절합니다. (높이 올라갈수록 X축으로 당겨서 시야 확보)
    float HeightAlpha = FMath::GetMappedRangeValueClamped(FVector2D(MinCameraHeight, MaxCameraHeight), FVector2D(0.f, 1.f), NewZ);
    float DynamicOffsetX = FMath::Lerp(-400.f, CameraOffset.X, HeightAlpha);
    
    // 5. 카메라가 최종적으로 가야 할 위치(FinalTarget)를 설정합니다.
    FVector FinalTarget = CenterTarget + FVector(DynamicOffsetX, 0.f, 0.f);
    FinalTarget.Z = NewZ;

    // 6. X, Y축도 부드럽게 이동하도록 보간을 적용해 최종 위치(NextLoc)를 적용합니다.
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
        // 🌟 1. 맵에 배치된 외부 카메라에서 캐릭터 내부의 '궁극기 카메라'로 부드럽게(0.2초) 화면을 넘깁니다.
        SetViewTargetWithBlend(MyChar, 0.2f, EViewTargetBlendFunction::VTBlend_Cubic);

        // 🌟 2. 시네마틱 카메라를 지탱하는 SpringArm의 구도를 연출에 맞게 조절합니다.
        if (USpringArmComponent* Arm = MyChar->GetUltSpringArm())
        {
            Arm->TargetArmLength = 350.0f; // 카메라를 캐릭터 쪽으로 당깁니다.
            Arm->SocketOffset = FVector(0.0f, 150.0f, 60.0f); // 캐릭터를 화면 약간 좌측/우측으로 치우치게 배치 (오프셋)
            
            if (TargetActor)
            {
                // 적과 나 사이의 방향 벡터를 구한 뒤, -90도를 회전시켜 '측면(Side) 구도'를 만듭니다.
                FVector Dir = TargetActor->GetActorLocation() - MyChar->GetActorLocation();
                Arm->SetWorldRotation(Dir.Rotation() + FRotator(0, -90, 0));
            }
        }
    }
    else
    {
        // 🌟 3. 연출이 끝나면 원래 사용하던 맵의 메인 카메라로 화면을 부드럽게(0.3초) 복구합니다.
        if (MainCameraActor)
        {
            SetViewTargetWithBlend(MainCameraActor, 0.3f, EViewTargetBlendFunction::VTBlend_Cubic);
            MainCameraActor->SetActorRotation(DefaultMainCameraRotation);
        }
    }
}