// Fill out your copyright notice in the Description page of Project Settings.


#include "FuriGameMode.h"

#include "FuriPlayerController.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/PlayerStart.h"
#include "GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AFuriGameMode::AFuriGameMode()
{
	SpawnedPlayerCount = 0;

	// 🌟 경로 형식을 표준 방식으로 수정합니다.
	static ConstructorHelpers::FObjectFinder<ULevelSequence> IntroSeq(TEXT("/Game/Cinematics/Sequences/StartCinematic/StartCinematicRoot"));
	if (IntroSeq.Succeeded())
	{
		IntroSequenceAsset = IntroSeq.Object;
		UE_LOG(LogTemp, Log, TEXT("[GameMode] IntroSequenceAsset 로드 성공!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] IntroSequenceAsset 로드 실패! 경로를 확인하세요."));
	}
}

void AFuriGameMode::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("[GameMode] BeginPlay 시작. 플레이어 대기 중..."));
	GetWorld()->GetTimerManager().SetTimer(CheckPlayersTimerHandle, this, &AFuriGameMode::CheckAllPlayersReady, 0.5f,
	                                       true);
}

UClass* AFuriGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// 1. 호스트(서버 리스너)인지 확인
	// 로컬 플레이어 컨트롤러이면서 서버 권한이 있다면 보통 호스트입니다.
	if (InController->IsLocalController())
	{
		return HostClass;
	}

	// 2. 그 외 접속자(클라이언트)에게는 다른 클래스를 반환
	return ClientClass;
}

APawn* AFuriGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	// 🌟 스폰 시 충돌을 무시하고 강제로 생성하도록 설정합니다 (AlwaysSpawn).
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Instigator = GetInstigator();

	UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
	if (PawnClass)
	{
		return GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnParams);
	}

	return nullptr;
}

AActor* AFuriGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// 1. 맵에 있는 모든 PlayerStart 액터를 찾아냅니다.
	TArray<AActor*> FoundPlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundPlayerStarts);

	// 2. 현재 몇 명째 스폰인지에 따라 찾을 태그(P1 또는 P2)를 결정합니다.
	FName TargetTag = (SpawnedPlayerCount == 0) ? FName("P1") : FName("P2");

	// 3. 찾아낸 PlayerStart들 중에서 태그가 일치하는 곳을 선택합니다.
	for (AActor* StartActor : FoundPlayerStarts)
	{
		APlayerStart* PStart = Cast<APlayerStart>(StartActor);
		if (PStart && PStart->PlayerStartTag == TargetTag)
		{
			// 태그가 일치하면 카운트를 올리고 해당 위치를 스폰 지점으로 반환합니다.
			SpawnedPlayerCount++;

			UE_LOG(LogTemp, Warning, TEXT("[GameMode] 플레이어 스폰 위치 확정: %s"), *TargetTag.ToString());
			return PStart;
		}
	}

	// 만약 P1, P2 태그를 못 찾았다면 엔진의 기본 스폰 로직을 따릅니다. (안전장치)
	UE_LOG(LogTemp, Error, TEXT("[GameMode] P1, P2 태그가 설정된 PlayerStart를 찾지 못했습니다!"));
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AFuriGameMode::CheckAllPlayersReady()
{
	int32 ReadyPlayers = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (PC->GetPawn() != nullptr)
			{
				ReadyPlayers++;
			}
		}
	}

	if (ReadyPlayers >= 2)
	{
		GetWorld()->GetTimerManager().ClearTimer(CheckPlayersTimerHandle);
		StartIntroSequence();
	}
}

void AFuriGameMode::StartIntroSequence()
{
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] 등장 시퀀스 시작!"));

	// 1. 모든 플레이어의 조작을 잠그고 캐릭터를 숨깁니다.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFuriPlayerController* PC = Cast<AFuriPlayerController>(It->Get());
		if (PC && PC->GetPawn())
		{
			// 🌟 카메라 페이드 아웃 (검은 화면 -> 밝음)
			PC->Client_FadeCamera(false, 1.0f);

			// 서버에서 숨김
			PC->GetPawn()->SetActorHiddenInGame(true);
			// 🌟 클라이언트에서도 숨기도록 명령
			PC->Client_SetActorHidden(PC->GetPawn(), true);

			AGasCharacterBase* Character = Cast<AGasCharacterBase>(PC->GetPawn());
			if (Character && Character->GetAbilitySystemComponent())
			{
				Character->GetAbilitySystemComponent()->AddLooseGameplayTag(
					FGameplayTag::RequestGameplayTag(FName("State.Lock")));
			}
		}
	}

	// 2. 레벨 시퀀스 재생
	if (IntroSequenceAsset)
	{
		ALevelSequenceActor* SequenceActor = nullptr;
		FMovieSceneSequencePlaybackSettings Settings;
		Settings.bAutoPlay = false;
		Settings.LoopCount.Value = 0; // 반복 없음 (1회 재생)

		// 시퀀스 플레이어 생성
		IntroSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
			GetWorld(), IntroSequenceAsset, Settings, SequenceActor);

		if (IntroSequencePlayer && SequenceActor)
		{
			// 🌟 서버의 시퀀스 재생을 모든 클라이언트에게 완벽하게 동기화합니다.
			SequenceActor->SetReplicates(true);
			SequenceActor->bReplicatePlayback = true;

			// 시퀀스가 끝나는 순간 OnCinematicFinished 함수가 자동으로 실행되도록 묶어줍니다(Bind).
			IntroSequencePlayer->OnFinished.AddDynamic(this, &AFuriGameMode::OnCinematicFinished);

			// 재생 시작!
			IntroSequencePlayer->Play();
		}
	}
	else
	{
		// 안전장치: 블루프린트에 시퀀스를 안 넣었다면 딜레이 없이 바로 전투 시작
		UE_LOG(LogTemp, Error, TEXT("[GameMode] IntroSequenceAsset이 없습니다! 바로 게임을 시작합니다."));
		OnCinematicFinished();
	}
}

void AFuriGameMode::OnCinematicFinished()
{
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] 시퀀스 종료! 캐릭터 표시 및 ReadyStart 표시"));

	// 1. 서버 및 클라이언트에서 플레이어 캐릭터를 다시 보이게 합니다.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFuriPlayerController* PC = Cast<AFuriPlayerController>(It->Get());
		if (PC && PC->GetPawn())
		{
			// 서버에서 보이기
			PC->GetPawn()->SetActorHiddenInGame(false);
			// 클라이언트에서도 보이도록 명령
			PC->Client_SetActorHidden(PC->GetPawn(), false);
		}
	}

	// 2. 서버에서 시네마틱 요소 숨김
	TArray<AActor*> CinematicActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Cinematic"), CinematicActors);
	for (AActor* Actor : CinematicActors)
	{
		if (Actor)
		{
			Actor->SetActorHiddenInGame(true);
			Actor->SetActorEnableCollision(false);
		}
	}

	// 3. 모든 클라이언트에게도 시네마틱 요소를 숨기도록 명령 (RPC) 및 UI 표시
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(It->Get()))
		{
			PC->Client_SetCinematicActorsHidden(true);
			PC->Client_ShowReadyStartUI();
		}
	}

	// 3. Level에 있는 PostProcessVolume의 infinite Extent를 true로 변경
	TArray<AActor*> FoundPPVs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APostProcessVolume::StaticClass(), FoundPPVs);
	for (AActor* Actor : FoundPPVs)
	{
		if (APostProcessVolume* PPV = Cast<APostProcessVolume>(Actor))
		{
			PPV->bUnbound = true; // Infinite Extent
		}
	}
}

void AFuriGameMode::EndIntroAndStartFight()
{
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] ReadyStart 종료! FIGHT!"));

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFuriPlayerController* PC = Cast<AFuriPlayerController>(It->Get());
		if (PC && PC->GetPawn())
		{
			// 🌟 서버에서 보이기
			PC->GetPawn()->SetActorHiddenInGame(false);
			// 🌟 클라이언트에서도 보이도록 명령
			PC->Client_SetActorHidden(PC->GetPawn(), false);

			// 1. 플레이어 화면에 FIGHT! UI 팝업
			PC->Client_ShowFightUI();
			
			// 2. HUD 타이머 시작
			PC->Client_StartGameHUDTimer();

			AGasCharacterBase* Character = Cast<AGasCharacterBase>(PC->GetPawn());
			if (Character && Character->GetAbilitySystemComponent())
			{
				// 3. 조작 잠금 해제 -> 이제 전투 가능!
				Character->GetAbilitySystemComponent()->RemoveLooseGameplayTag(
					FGameplayTag::RequestGameplayTag(FName("State.Lock")));
			}
		}
	}
}

// 0408 추가 작업중

void AFuriGameMode::ProcessMatchEnd(AGasCharacterBase* Winner, AGasCharacterBase* Loser)
{
	CachedWinner = Winner;
	CachedLoser = Loser;
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(It->Get()))
		{
			PC->Client_PlayFinishingSequence(Loser,false);
		}
	}
}

void AFuriGameMode::OnDeathAnimationFinished()
{
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(It->Get()))
		{
			PC->Client_PlayFinishingSequence(CachedWinner, true);
		}
	}
}

void AFuriGameMode::OnVictoryAnimationFinished()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(It->Get()))
		{
			bool bIsWinner = (PC->GetPawn() == CachedWinner);
			PC->Client_FinalShowUI(bIsWinner);
		}
	}
}

