#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FuriTypes.generated.h"

// 데미지 타입: 속성 및 판정 구분
UENUM(BlueprintType)
enum class EFuriDamageType : uint8
{
	None            UMETA(DisplayName = "None"),
	Melee           UMETA(DisplayName = "Melee"),      // 근접
	Projectile      UMETA(DisplayName = "Projectile"), // 투사체
	Explosion       UMETA(DisplayName = "Explosion"),  // 폭발
	Environment     UMETA(DisplayName = "Environment") // 환경 요소
};

// 데미지 반응: 피격 시 애니메이션 및 상태 결정
UENUM(BlueprintType)
enum class EFuriDamageResponse : uint8
{
	None            UMETA(DisplayName = "None"),
	HitReaction     UMETA(DisplayName = "Hit Reaction"), // 일반 피격
	Stagger         UMETA(DisplayName = "Stagger"),      // 경직 (비틀거림)
	Stun            UMETA(DisplayName = "Stun"),         // 기절
	KnockBack       UMETA(DisplayName = "Knock Back")    // 밀려남
};

// 데미지 정보 구조체: GAS의 EffectContext 등에 실어 보낼 데이터
USTRUCT(BlueprintType)
struct FFuriDamageInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Amount = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFuriDamageType DamageType = EFuriDamageType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFuriDamageResponse DamageResponse = EFuriDamageResponse::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShouldDamageInvincible = false; // 무적 무시 여부

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanBeBlocked = false;           // 가드 가능 여부

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanBeParried = false;           // 패링 가능 여부

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShouldForceInterrupt = false;  // 강제 인터럽트(시전 취소) 여부
};

// 개별 무기에 대한 설정
USTRUCT(BlueprintType)
struct FWeaponSlotConfig
{
	GENERATED_BODY()

	// 사용할 스태틱 메쉬
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMesh> WeaponMesh;

	// 캐릭터의 어느 소켓에 붙일 것인가?
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AttachSocketName;
	
	// 캐릭터마다 무기 크기를 다르게 설정할 수 있도록 변수 추가
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector WeaponScale = FVector(1.0f);
};

//무기변경에 대한 전체 설정
USTRUCT(BlueprintType)
struct FWeaponConfig
{
	GENERATED_BODY()

	// 무기 식별 태그 (예: Weapon.DualKatana)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag WeaponTag;

	// 무기 슬롯
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FWeaponSlotConfig> WeaponSlots;

	// 이 무기 세트를 들었을 때 적용할 애니메이션 레이어 (ABP)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> AnimLayerClass;

	// 이 무기를 들었을 때의 이동 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxWalkSpeed = 800.f;
};

//먼저 "어떤 입력 태그"가 "어떤 Gameplay Tag"와 매핑되는지 정의
USTRUCT(BlueprintType)
struct FFuriInputActionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<class UInputAction> InputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag InputTag; // 예: InputTag.Ability.Dash
};