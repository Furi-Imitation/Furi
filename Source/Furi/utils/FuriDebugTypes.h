#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FuriDebugTypes.generated.h"

UENUM(BlueprintType)
enum class EFuriPredictionStatus : uint8
{
    None,
    Predicted,
    Confirmed,
    Rejected
};

USTRUCT(BlueprintType)
struct FFuriPredictionDebugData
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float Timestamp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString AbilityName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EFuriPredictionStatus Status;

    FFuriPredictionDebugData() : Timestamp(0.0f), Status(EFuriPredictionStatus::None) {}
};

USTRUCT(BlueprintType)
struct FFuriTagDebugData
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FGameplayTag Tag;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Count;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FColor DisplayColor;
};

USTRUCT(BlueprintType)
struct FFuriNetSerializeDebugData
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 RawBytes;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 OptimizedBytes;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float ReductionPercentage;
};

USTRUCT(BlueprintType)
struct FFuriComboDebugData
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 CurrentComboIndex;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bWindowOpened;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bReserved;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString LastFailureReason;
};
