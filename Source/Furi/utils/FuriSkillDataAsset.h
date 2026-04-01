#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "FuriTypes.h"
#include "FuriSkillDataAsset.generated.h"

class UTexture2D;
class UAnimMontage;
class UGameplayAbility;

// 🌟 스킬 하나의 '모든' 정보를 담는 만능 구조체
USTRUCT(BlueprintType)
struct FFuriSkillData
{
	GENERATED_BODY()

	// ==========================================
	// UI 및 시각 정보
	// ==========================================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "1. UI")
	FText SkillName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "1. UI")
	UTexture2D* SkillIcon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "1. UI")
	FText SkillDescription;

	// ==========================================
	//  GAS 시스템 및 자원 정보
	// ==========================================
	// 이 스킬을 실행할 실제 어빌리티 클래스 (GA)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "2. System")
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "2. System")
	float StaminaCost = 0.f;

	// UI에서 쿨타임을 추적하기 위한 태그 (예: Cooldown.Skill.Ultimate)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "2. System")
	FGameplayTag CooldownTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "2. System")
	float CooldownDuration = 0.f;

	// 🌟 작성해주신 완벽한 대미지 정보 구조체 삽입!
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "3. Combat")
	FFuriDamageInfo DamageInfo;
};

UCLASS(BlueprintType)
class FURI_API UFuriSkillDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skills")
	TMap<FGameplayTag, FFuriSkillData> SkillDataMap;

	// 태그를 넣어 해당 스킬의 데이터를 안전하게 꺼내오는 헬퍼 함수
	UFUNCTION(BlueprintCallable, Category = "Skills")
	bool GetSkillDataByTag(FGameplayTag SkillTag, FFuriSkillData& OutData) const;
};
