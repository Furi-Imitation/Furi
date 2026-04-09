// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GasCharacterBase.generated.h"

class UFuriSkillDataAsset;
struct FFuriDamageInfo;
class USpringArmComponent;
class UCameraComponent;
enum class EFuriDamageResponse : uint8;
class UBasicAttributeSet;
struct FBranchingPointNotifyPayload;

UCLASS()
class FURI_API AGasCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGasCharacterBase();

	// Ability System Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	UBasicAttributeSet* BasicAttributeSet;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability System")
	EGameplayEffectReplicationMode AscReplicationMode = EGameplayEffectReplicationMode::Mixed;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// 🌟 피격 반응을 통합 처리하는 핵심 함수 (Attacker를 받아 날아갈 방향을 계산함)
	void HandleDamageResponse(const FFuriDamageInfo& DamageInfo, AActor* Attacker = nullptr);

protected:
	// --- 리액션 몽타주 ---
	UPROPERTY(EditAnywhere, Category = "Furi | Design | Animation")
	TObjectPtr<UAnimMontage> HitReactionMontage;

	UPROPERTY(EditAnywhere, Category = "Furi | Design | Animation")
	TObjectPtr<UAnimMontage> StaggerMontage;

	UPROPERTY(EditAnywhere, Category = "Furi | Design | Animation")
	TObjectPtr<UAnimMontage> StunMontage;

	UPROPERTY(EditAnywhere, Category = "Furi | Design | Animation")
	TObjectPtr<UAnimMontage> KnockBackMontage;

	// --- 🌟 궁극기 연출 전용 카메라 컴포넌트 ---
	// 에디터에서 값을 확인할 수 있고 블루프린트에서 읽기 전용으로 설정합니다.

	/** 궁극기 시점을 담당할 SpringArm */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ultimate | Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> UltSpringArm;

	/** 실제 시점이 될 Camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ultimate | Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> UltCamera;

public:
	// --- 유틸리티: 생성된 컴포넌트의 게터(Getter) ---
	FORCEINLINE USpringArmComponent* GetUltSpringArm() const { return UltSpringArm; }
	FORCEINLINE UCameraComponent* GetUltCamera() const { return UltCamera; }

protected:
	// 연속 피격 시 기존 타이머를 취소하기 위해 멤버 변수로 선언
	FTimerHandle StunTimerHandle;
	FTimerHandle LockTimerHandle;

	// 🌟 서버가 모든 클라이언트에게 몽타주 재생을 명령하는 함수
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayReactionMontage(UAnimMontage* MontageToPlay);

	// 🌟 서버가 모든 클라이언트에게 몽타주 정지를 명령하는 함수
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_StopReactionMontage(UAnimMontage* MontageToStop);

	// 히트스탑 중인지 확인하는 플래그
	bool bIsHitStopping = false;

	// 히트스탑이 걸리기 직전의 원래 시간 배속 (예: 궁극기 중이라면 0.3)
	float PreHitStopTimeDilation = 1.0f;

	// 현재 실행 중인 티커 핸들 (연속 타격 시 이전 티커를 취소하기 위함)
	FTSTicker::FDelegateHandle HitStopTickerHandle;

	void ExecuteHitStop(float Duration, float TimeScale = 0.01f);
	void StopHitStop();
	
protected:
	// 🌟 이 캐릭터의 스킬 UI 데이터를 담은 에셋 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data|Skill")
	UFuriSkillDataAsset* SkillUIData;

public:
	// UI(HUD)가 이 캐릭터의 스킬 데이터를 가져갈 수 있도록 열어주는 Getter 함수
	UFUNCTION(BlueprintCallable, Category = "Data|Skill")
	UFuriSkillDataAsset* GetSkillUIData() const { return SkillUIData; }
	
public:
	// 🌟 AttributeSet에서 체력이 0이 되었을 때 호출할 사망 함수
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void Die();
	
	// 🌟 이 함수들을 추가해야 Controller에서 호출할 수 있습니다.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PlayVictoryMontage();
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PlayDeathMontage();
	
	// UFUNCTION()
	//void OnFinishMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	bool bIsWinningSequence = false;
protected:
	// 사망 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Montages")
	UAnimMontage* DeathMontage;
	
	// 승리 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Montages")
	UAnimMontage* VictoryMontage;
	

	// 모든 클라이언트에서 사망 처리 (애니메이션, 물리 비활성화 등)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Die();
	
public:
	UFUNCTION()
	void AnimNotify_DieFinish();

	// 🌟 서버에게 노티파이 발생을 알리는 함수
	UFUNCTION(Server, Reliable)
	void Server_NotifyDieFinish();
	
	UFUNCTION() 
	void AnimNotify_VictoryFinish();
	UFUNCTION(Server, Reliable) 
	void Server_NotifyVictoryFinish();

	UFUNCTION()
	void HandleMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);

private:
	void BindFinishingMontageNotifies();

	bool bDeathFinishNotified = false;
	bool bVictoryFinishNotified = false;
};
