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
	class UButton* ReadyButton;
	
	UPROPERTY(meta = (BindWidget))
	class UButton* ExitButton;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* WaitingText;
	
	UFUNCTION()
	void OnStartButtonClicked();
	
	UFUNCTION()
	void OnQuitClicked();
	
	// 서버/클라이언트 공용: Ready 상태 업데이트 함수 (UI 반영용)
	void UpdateReadyVisual(bool bIsReady);
    
	// 두 명 모두 레디했을 때 호출될 함수
	void ShowWaitingMessage(bool bShow);
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UFuriGameHUDWidget> FuriGameHUDWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UFuriGameHUDWidget> FuriSkillIconWidgetClass;
};
