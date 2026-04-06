// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "FuriPlayerController.generated.h"

class UFuriGameHUDWidget;
/**
 * 
 */
UCLASS()
class FURI_API AFuriPlayerController : public APlayerController
{
	GENERATED_BODY()
    
public:
	AFuriPlayerController();
    
	void SetCinematicMode(bool bEnabled, AActor* TargetActor = nullptr);
	void UpdateStandardCamera(float DeltaTime);
	

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
    
private:
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraInterpSpeed = 7.0f;
	UPROPERTY(EditAnywhere, Category = "Camera")
	float ZoomInterpSpeed = 5.0f;
	UPROPERTY(EditAnywhere, Category = "Camera")
	float MinCameraHeight = 400.0f;
	UPROPERTY(EditAnywhere, Category = "Camera")
	float MaxCameraHeight = 3000.0f;
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraPadding = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector CameraOffset = FVector(-3000.f, 0.f, 0.f);

	UPROPERTY()
	AActor* MainCameraActor;

	bool bIsCinematicMode = false;
	UPROPERTY()
	AActor* CinematicTarget = nullptr;

	// 복구용 기본값
	FRotator DefaultMainCameraRotation;
	
	
protected:
	//HUD 생성
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UFuriGameHUDWidget> MainHUDWidgetClass;

	// 🌟 실제로 화면에 띄워진 위젯의 메모리를 쥐고 있을 포인터
	UPROPERTY(Transient)
	TObjectPtr<UFuriGameHUDWidget> MainHUDWidget;
	
	// 🌟 적 탐색을 위한 타이머 핸들
	FTimerHandle EnemySearchTimerHandle;

	// 🌟 적을 찾아서 UI에 연결 시도하는 함수
	void TryFindEnemyForHUD();
	

protected:
    // 에디터(BP_FuriPlayerController)에서 WBP_EndUI를 선택할 변수
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<class UEndUI> EndUIClass;

    // 생성된 UI 인스턴스를 보관할 포인터
    UPROPERTY()
    class UEndUI* EndUIInstance;

public:
    // 외부(캐릭터 사망 등)에서 호출할 게임 종료 함수
    void ShowGameEndUI(bool bVictory);
	
	// NetMulticast나 Client 키워드를 사용하여 서버가 클라이언트에게 명령하게 합니다.
	UFUNCTION(Client, Reliable)
	void Client_ShowGameEndUI(bool bVictory);
	
public:
	// 서버에서 실행될 RPC (클라이언트가 서버에게 레디를 알림)
	UFUNCTION(Server, Reliable)
	void Server_SetReady(bool bNewReady);

	// 준비 상태 확인 함수 (Getter)
	FORCEINLINE bool IsReady() const { return bIsReady; }

protected:
	// 속성 복제 설정 (서버의 값이 클라이언트로 자동 전달됨)
	UPROPERTY(Replicated)
	bool bIsReady = false;

	// 네트워크 복제 규칙을 정의하는 필수 함수
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
