// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GasCharacterBase.generated.h"

class USpringArmComponent;
class UCameraComponent;
enum class EFuriDamageResponse : uint8;
class UBasicAttributeSet;

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
	
protected:
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
	
public:
	// 🌟 피격 반응을 통합 처리하는 핵심 함수 (Attacker를 받아 날아갈 방향을 계산함)
	void HandleDamageResponse(EFuriDamageResponse Response, AActor* Attacker = nullptr);

protected:
	// --- 리액션 몽타주 리스트 ---
	UPROPERTY(EditAnywhere, Category = "Furi | Design | Animation")
	TObjectPtr<UAnimMontage> HitReactionMontage;

	UPROPERTY(EditAnywhere, Category = "Furi | Design | Animation")
	TObjectPtr<UAnimMontage> StaggerMontage;

	UPROPERTY(EditAnywhere, Category = "Furi | Design | Animation")
	TObjectPtr<UAnimMontage> StunMontage;

	UPROPERTY(EditAnywhere, Category = "Furi | Design | Animation")
	TObjectPtr<UAnimMontage> KnockBackMontage;

	UPROPERTY(EditAnywhere, Category = "Furi | Design | Animation")
	TObjectPtr<UAnimMontage> GuardReactionMontage; // 가드 성공 시 모션

protected:
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
};
