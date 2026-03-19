#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Furi/utils/FuriTypes.h"
#include "WeaponManagerComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FURI_API UWeaponManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponManagerComponent();

	// 실제 무기를 장착시키는 핵심 함수
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeapon(const FWeaponConfig& NewConfig);

protected:
	// 현재 캐릭터가 들고 있는 무기 메쉬들을 저장 (나중에 지우거나 참조하기 위함)
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> EquippedWeaponMeshes;
};
