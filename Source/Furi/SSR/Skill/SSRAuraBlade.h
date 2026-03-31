#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SSRAuraBlade.generated.h"

UCLASS()
class FURI_API USSRAuraBlade : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USSRAuraBlade();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 1초 뒤에 실행될 실제 발사 로직
	UFUNCTION()
	void OnDelayFinished();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AuraBlade|Projectile")
	TSubclassOf<class AAuraBladeProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AuraBlade|Damage")
	TSubclassOf<class UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AuraBlade|Balance")
	float AbilityDamage = 20.f;

	// 대기 시간 (1.0f = 1초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AuraBlade|Balance")
	float DelayTime = 0.8f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AuraBlade|Animation")
	TObjectPtr<UAnimMontage> ChargeMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AuraBlade|Effects")
	FGameplayTag ChargeCueTag; // 1초 동안 보여줄 기 모으기 효과

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AuraBlade|Effects")
	FGameplayTag FireCueTag;   // 발사 시 효과
};
