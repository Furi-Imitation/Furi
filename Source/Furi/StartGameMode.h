// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StartGameMode.generated.h"

UCLASS()
class FURI_API AFuriStartGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	// 블루프린트에서 StartUI 위젯 클래스를 할당하기 위함
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> StartUIClass;

	UPROPERTY()
	UUserWidget* StartUIInstance;
};
