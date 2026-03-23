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
	class UInputAction* IA_SSRDash;
	
	UPROPERTY(EditDefaultsOnly, Category = Input)
	class UInputAction* IA_SSRSunFire;
	
	UPROPERTY(EditDefaultsOnly, Category = Input)
	class UInputAction* IA_SSRSkill2;
	
	UPROPERTY(EditDefaultsOnly, Category = Input)
	class UInputAction* IA_SSRSkill3;
	
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
	void Dash (const FInputActionValue& InputValue);
	void SunFire (const FInputActionValue& InputValue);
	void SwordSkill2 (const FInputActionValue& InputValue);
	void SwordSkill3 (const FInputActionValue& InputValue);
	
	
	// Gameplay Ability System
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Weapons)
	class UWeaponManagerComponent* WeaponManager;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Weapons)
	TObjectPtr<class UWeaponDataAsset> DefaultWeaponData;
	
protected:
	virtual void PossessedBy(AController* NewController) override;
	
	// 부여할 어빌리티 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	// 초기화 함수 (주로 PossessedBy에서 호출)
	void InitAbilityActorInfo();
	
protected:
	// 에디터에서 IA와 Tag를 매핑한 리스트를 담을 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TArray<FFuriInputActionConfig> AbilityInputConfigs;

	// 입력 처리 함수
	void AbilityInputTagPressed(FGameplayTag InputTag);
	
protected:
	// 게임 시작 시 자동으로 걸어줄 스테미나 재생 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities|Effects")
	TSubclassOf<class UGameplayEffect> StaminaRegenEffectClass;
};
