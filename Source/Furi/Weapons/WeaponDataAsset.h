// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Furi/utils/FuriTypes.h"
#include "WeaponDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class FURI_API UWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	// 이 에셋이 들고 있을 무기 설정 데이터
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FWeaponConfig WeaponConfig;
};
