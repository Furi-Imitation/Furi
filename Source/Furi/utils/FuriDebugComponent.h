#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FuriDebugTypes.h"
#include "FuriDebugComponent.generated.h"

class UAbilitySystemComponent;
class UGA_Attack;
class UGameplayAbility;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FURI_API UFuriDebugComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UFuriDebugComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void UpdateNetSerializeData(int32 Raw, int32 Optimized);
    void UpdateComboData(int32 Index, bool bOpened, bool bReserved, FString FailureReason = "");

	UFUNCTION(BlueprintPure, Category = "Furi|Debug")
	const TArray<FFuriPredictionDebugData>& GetPredictionHistory() const { return PredictionHistory; }

	UFUNCTION(BlueprintPure, Category = "Furi|Debug")
	const TArray<FFuriTagDebugData>& GetActiveTags() const { return ActiveTags; }

	UFUNCTION(BlueprintPure, Category = "Furi|Debug")
	FFuriNetSerializeDebugData GetNetSerializeData() const { return NetSerializeData; }

	UFUNCTION(BlueprintPure, Category = "Furi|Debug")
	FFuriComboDebugData GetComboData() const { return ComboData; }

private:
	void OnAbilityActivated(UGameplayAbility* Ability);
	void OnTagChanged(const FGameplayTag Tag, int32 NewCount);

	UPROPERTY()
	UAbilitySystemComponent* ASC;

	TArray<FFuriPredictionDebugData> PredictionHistory;
	TArray<FFuriTagDebugData> ActiveTags;
	FFuriNetSerializeDebugData NetSerializeData;
	FFuriComboDebugData ComboData;

	static constexpr int32 MaxPredictionHistory = 10;
};
