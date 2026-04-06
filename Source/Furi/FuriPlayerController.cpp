#include "FuriPlayerController.h"

#include "FuriGameMode.h"
#include "Net/UnrealNetwork.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "UI/FuriGameHUDWidget.h"
#include "SSR/UI/EndUI.h"

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
		// ==========================================
		// 1. 카메라 초기화 세팅
		// ==========================================
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

		// ==========================================
		// 2. UI (HUD) 생성 및 데이터 연동
		// ==========================================
		// 2. UI 생성 및 내 캐릭터 연동 (기존 동일)
		
		
		// startui랑 겹쳐서 나오는중
		
		if (MainHUDWidgetClass)
		{
			MainHUDWidget = CreateWidget<UFuriGameHUDWidget>(this, MainHUDWidgetClass);
			if (MainHUDWidget)
			{
				MainHUDWidget->AddToViewport();
		
				if (AGasCharacterBase* MyChar = Cast<AGasCharacterBase>(GetPawn()))
				{
					MainHUDWidget->InitPlayerStats(MyChar->GetAbilitySystemComponent());
				}
		
				// 여기서 맵을 한 번만 뒤지는 대신, 타이머를 가동합니다!
				// 0.5초마다 TryFindEnemyForHUD 함수를 반복 실행합니다.
				GetWorldTimerManager().SetTimer(EnemySearchTimerHandle, this,
				                                &AFuriPlayerController::TryFindEnemyForHUD, 0.5f, true);
			}
		}
		// --------------------------------------
	}
}

void AFuriPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!IsLocalPlayerController() || !MainCameraActor)
	{
		return;
	}

	// 시네마틱 연출 모드일 때는 카메라가 플레이어들을 쫓아다니지 않도록 막습니다.
	if (bIsCinematicMode)
	{
		return;
	}

	// 일반 전투 상황이라면 두 플레이어를 화면에 담기 위해 카메라 위치를 업데이트합니다.
	UpdateStandardCamera(DeltaTime);
}

void AFuriPlayerController::UpdateStandardCamera(float DeltaTime)
{
	// 맵에 있는 모든 캐릭터(플레이어와 적)를 가져옵니다.
	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), Players);
	if (Players.Num() == 0)
	{
		return;
	}

	// 1. 모든 캐릭터의 위치를 더해 중간 지점(CenterTarget)을 계산합니다.
	FVector SumLocation = FVector::ZeroVector;
	for (AActor* Actor : Players) { SumLocation += Actor->GetActorLocation(); }
	FVector CenterTarget = SumLocation / Players.Num();

	// 2. 캐릭터들 사이의 가장 먼 거리를 구합니다. (1vs1 기준)
	float MaxDist = (Players.Num() >= 2)
		                ? FVector::Dist(Players[0]->GetActorLocation(), Players[1]->GetActorLocation())
		                : 0.f;

	FVector CurrentCamLoc = MainCameraActor->GetActorLocation();

	// 3. 거리에 비례하여 카메라의 목표 높이(TargetZ)를 계산하고, 최소/최대 높이로 제한(Clamp)합니다.
	float TargetZ = FMath::Clamp(MinCameraHeight + (MaxDist * CameraPadding), MinCameraHeight, MaxCameraHeight);

	// 현재 높이에서 목표 높이로 부드럽게 이동(Interpolation)합니다.
	float NewZ = FMath::FInterpTo(CurrentCamLoc.Z, TargetZ, DeltaTime, ZoomInterpSpeed);

	// 4. 높이에 따라 X축 오프셋을 동적으로 조절합니다. (높이 올라갈수록 X축으로 당겨서 시야 확보)
	float HeightAlpha = FMath::GetMappedRangeValueClamped(FVector2D(MinCameraHeight, MaxCameraHeight),
	                                                      FVector2D(0.f, 1.f), NewZ);
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
	if (!MyChar)
	{
		return;
	}

	if (bEnabled)
	{
		// 1. 맵에 배치된 외부 카메라에서 캐릭터 내부의 '궁극기 카메라'로 부드럽게(0.2초) 화면을 넘깁니다.
		SetViewTargetWithBlend(MyChar, 0.2f, VTBlend_Cubic);
	}
	else
	{
		// 3. 연출이 끝나면 원래 사용하던 맵의 메인 카메라로 화면을 부드럽게(0.3초) 복구합니다.
		if (MainCameraActor)
		{
			SetViewTargetWithBlend(MainCameraActor, 0.3f, VTBlend_Cubic);
			MainCameraActor->SetActorRotation(DefaultMainCameraRotation);
		}
	}
}

// 적 탐색 함수
void AFuriPlayerController::TryFindEnemyForHUD()
{
	// 아직 UI가 없거나, 내 캐릭터가 없다면 탐색 보류
	if (!MainHUDWidget || !GetPawn())
	{
		return;
	}

	TArray<AActor*> FoundCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGasCharacterBase::StaticClass(), FoundCharacters);

	for (AActor* Actor : FoundCharacters)
	{
		AGasCharacterBase* PotentialEnemy = Cast<AGasCharacterBase>(Actor);

		// 찾은 캐릭터가 유효하고, '나' 자신이 아니며, 적의 ASC가 정상적으로 생성(복제)되었다면
		if (PotentialEnemy && PotentialEnemy != GetPawn() && PotentialEnemy->GetAbilitySystemComponent())
		{
			// UI에 연동
			MainHUDWidget->InitEnemyStats(PotentialEnemy->GetAbilitySystemComponent());

			UE_LOG(LogTemp, Log, TEXT("[UI] Delayed Enemy ASC Linked Success: %s"), *PotentialEnemy->GetName());

			// 목적을 달성했으므로 더 이상 탐색하지 않도록 타이머를 파괴합니다.
			GetWorldTimerManager().ClearTimer(EnemySearchTimerHandle);
			break;
		}
	}
}


// 승리 패배 ui
void AFuriPlayerController::ShowGameEndUI(bool bVictory)
{
	UE_LOG(LogTemp, Warning, TEXT("ShowGameEndUI Attempt..."));

	if (!EndUIClass) 
	{
		UE_LOG(LogTemp, Error, TEXT("EndUIClass is STILL NULL! Check BP again!"));
		return;
	}

	if (EndUIClass)
	{
		EndUIInstance = CreateWidget<UEndUI>(this, EndUIClass);
		if (EndUIInstance)
		{
			EndUIInstance->AddToViewport(100); // 다른 UI보다 앞에 나오도록 높은 우선순위
            
			// C++에서 만든 함수 호출 (내부적으로 블루프린트 이벤트 OnGameResultDetermined 실행)
			EndUIInstance->SetGameResult(bVictory);
			
			UE_LOG(LogTemp, Warning, TEXT("EndUIInstance Added to Viewport!"))
			

			// 마우스 커서를 보이게 하고 UI에 포커스를 맞춥니다.
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(EndUIInstance->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            
			SetInputMode(InputMode);
			bShowMouseCursor = true;

			// 게임의 움직임을 멈추고 싶다면 아래 주석을 해제하세요.
			// UGameplayStatics::SetGamePaused(GetWorld(), true);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to Create EndUIInstance!"));
	}
}

void AFuriPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// bIsReady 변수를 복제 목록에 추가
	DOREPLIFETIME(AFuriPlayerController, bIsReady);
}

void AFuriPlayerController::Server_SetReady_Implementation(bool bNewReady)
{
	// 서버에서 값을 변경하면 Replicated 설정 덕분에 모든 클라이언트에게 전달됨
	bIsReady = bNewReady;
	UE_LOG(LogTemp, Warning, TEXT("Server Received Ready from Player!"));

	// 서버의 GameMode에게 모두가 레디했는지 체크하라고 시킴
	AFuriGameMode* GM = Cast<AFuriGameMode>(GetWorld()->GetAuthGameMode());
	if (GM)
	{
		GM->CheckAllPlayersReady();
	}
}

void AFuriPlayerController::Client_ShowGameEndUI_Implementation(bool bVictory)
{
	// 이 코드는 이제 각 플레이어의 '진짜 자기 컴퓨터'에서만 실행됩니다.
	ShowGameEndUI(bVictory);
}
