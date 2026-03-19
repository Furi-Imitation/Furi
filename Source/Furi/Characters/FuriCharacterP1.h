// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "FuriCharacterP1.generated.h"

struct FFuriInputActionConfig;
class UWeaponDataAsset;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
UCLASS()
class FURI_API AFuriCharacterP1 : public AGasCharacterBase
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
public:
	// Sets default values for this character's properties
	AFuriCharacterP1();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> IMC_P1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MouseLookAction;
	
	void Move(const FInputActionValue& inputValue);
	void Look(const FInputActionValue& inputValue);
	void DoMove(float Right, float Forward);
	void DoLook(float Yaw, float Pitch);
	
public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	
protected:
	//무기 관리 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	class UWeaponManagerComponent* WeaponManager;
	
	//시작 시 장착할 기본 무기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<UWeaponDataAsset> DefaultWeaponData;
	
public:
	
	UWeaponManagerComponent* GetWeaponManager() const {return WeaponManager;}
	
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
