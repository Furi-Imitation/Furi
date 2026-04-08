// Fill out your copyright notice in the Description page of Project Settings.


#include "FuriGameMode.h"

#include "FuriPlayerController.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "GameFramework/PlayerStart.h"
#include "GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Kismet/GameplayStatics.h"

AFuriGameMode::AFuriGameMode()
{
	SpawnedPlayerCount = 0;
}

void AFuriGameMode::BeginPlay()
{
	Super::BeginPlay();
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

	// 1. 모든 플레이어의 조작을 잠급니다 (Lock 태그 부여)
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFuriPlayerController* PC = Cast<AFuriPlayerController>(It->Get());
		if (PC && PC->GetPawn())
		{
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

			// 시퀀스가 끝나는 순간 EndIntroAndStartFight 함수가 자동으로 실행되도록 묶어줍니다(Bind).
			IntroSequencePlayer->OnFinished.AddDynamic(this, &AFuriGameMode::EndIntroAndStartFight);

			// 재생 시작!
			IntroSequencePlayer->Play();
		}
	}
	else
	{
		// 안전장치: 블루프린트에 시퀀스를 안 넣었다면 딜레이 없이 바로 전투 시작
		UE_LOG(LogTemp, Error, TEXT("[GameMode] IntroSequenceAsset이 없습니다! 바로 게임을 시작합니다."));
		EndIntroAndStartFight();
	}
}

void AFuriGameMode::EndIntroAndStartFight()
{
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] 시퀀스 종료! FIGHT!"));

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFuriPlayerController* PC = Cast<AFuriPlayerController>(It->Get());
		if (PC && PC->GetPawn())
		{
			// 1. 플레이어 화면에 FIGHT! UI 팝업
			PC->Client_ShowFightUI();

			AGasCharacterBase* Character = Cast<AGasCharacterBase>(PC->GetPawn());
			if (Character && Character->GetAbilitySystemComponent())
			{
				// 2. 조작 잠금 해제 -> 이제 전투 가능!
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

