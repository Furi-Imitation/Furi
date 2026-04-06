// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StartUI.generated.h"

UCLASS()
class FURI_API UStartUI : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

protected:
	UPROPERTY(meta = (BindWidget))
	class UButton* StartButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* ExitButton;

	// (옵션) 버튼 안의 텍스트를 "준비 완료!" 등으로 바꾸기 위해 바인딩
	// WBP에서 TextBlock 이름을 StartButtonText로 맞추면 자동으로 연결됩니다.
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* StartButtonText;

	UFUNCTION()
	void OnStartButtonClicked();

	UFUNCTION()
	void OnQuitClicked();
};
