#pragma once

#include "CoreMinimal.h"
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