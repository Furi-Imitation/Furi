// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FuriBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class FURI_API UFuriBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/**
	 * 지정한 위치를 중심으로 가장 가까운 적을 찾습니다.
	 * @param Searcher 탐색을 수행하는 주체 (자신은 타겟에서 제외됨)
	 * @param SearchRadius 탐색 반경
	 * @param ActorsToIgnore 무시할 액터 배열 (선택 사항)
	 * @return 반경 내에서 가장 가까운 타겟 액터 (없으면 nullptr)
	 */
	UFUNCTION(BlueprintCallable, Category = "Furi|Combat")
	static AActor* FindClosestTarget(AActor* Searcher, float SearchRadius, const TArray<AActor*>& ActorsToIgnore);
	
	/**
	 * 타겟의 주변 사각지대(뒤, 양옆) 좌표와 타겟을 바라보는 회전값을 계산합니다.
	 * @param Target 기준이 되는 타겟 액터
	 * @param StrikeIndex 콤보 타수 (1: 등 뒤, 2: 우측면, 3: 좌측면)
	 * @param Distance 타겟과 띄울 거리 (너무 0이면 몸이 겹침)
	 * @param OutLocation 계산된 텔레포트 목표 위치
	 * @param OutRotation 계산된 텔레포트 목표 회전 (항상 타겟을 바라봄)
	 */
	UFUNCTION(BlueprintPure, Category = "Furi|Combat")
	static void CalculateTeleportTransform(AActor* Target, int32 StrikeIndex, float Distance, FVector& OutLocation, FRotator& OutRotation);
};
