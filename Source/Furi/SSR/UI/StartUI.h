// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StartUI.generated.h"

/**
 * 
 */
UCLASS()
class FURI_API UStartUI : public UUserWidget
{
	GENERATED_BODY()

	
public:
	virtual bool Initialize() override;
	
	UPROPERTY(meta = (BindWidget))
	class UButton* StartButton;
	
	UPROPERTY(meta = (BindWidget))
	class UButton* ExitButton;
	
	UFUNCTION()
	void OnStartButtonClicked();
	
	UFUNCTION()
	void OnQuitClicked();
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UFuriGameHUDWidget> FuriGameHUDWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UFuriGameHUDWidget> FuriSkillIconWidgetClass;
};
