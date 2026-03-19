// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "SSRSword.h"
#include "SSRPlayer.generated.h"

/**
 * 
 */
UCLASS()
class FURI_API ASSRPlayer : public AGasCharacterBase
{
	GENERATED_BODY()
	
public:
	ASSRPlayer();
	
protected:
	virtual void BeginPlay() override;
	
public:
	virtual void Tick(float DeltaTime) override;

	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

public:
	UPROPERTY(EditDefaultsOnly, Category = Input)
	class UInputAction* IA_SSRMove;
	
	UPROPERTY(EditDefaultsOnly, Category = Input)
	class UInputAction* IA_SSRSwordAttack;
	
	UPROPERTY(EditDefaultsOnly, Category = Input)
	class UInputAction* IA_SSRShieldBlock;
	
	UPROPERTY(EditDefaultsOnly, Category = Input)
	class UInputMappingContext* IMC_SSR;
	
	// 플레이어 움직임 처리
	void Move(const FInputActionValue& InputActionValue);
	
	FVector Direction;
	float MovementSpeed = 1000.f;
	
	// 칼과 방패
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	class USkeletalMeshComponent* SwordMeshComp;
	
	void SwordAttack(const FInputActionValue& InputValue);
	
	UPROPERTY(EditAnywhere, CateGory = Weapon)
	TSubclassOf<class ASSRSword> SwordClass;
	ASSRSword* CurrentWeapon;
	
	
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	class USkeletalMeshComponent* ShieldMeshComp;
	
	void SheildBlock(const FInputActionValue& InputValue);
	
	
};
