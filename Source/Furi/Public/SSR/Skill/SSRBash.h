// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SSRBash.generated.h"

/**
 * 플레이어가 스킬을 쓰면 방패로 공격한다
 * 적에게 데미지를 주고 기절(Stun)을 적용한다
 */
UCLASS()
class FURI_API USSRBash : public UGameplayAbility
{
	GENERATED_BODY()
};
