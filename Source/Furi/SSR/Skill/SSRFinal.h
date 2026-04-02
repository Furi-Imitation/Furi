#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "SSRFinal.generated.h"

UCLASS()
class FURI_API USSRFinal : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USSRFinal();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 몽타주 노티파이에서 쏠 통합 태그 (예: Event.SSR.Final.Hit) */
	UFUNCTION()
	void OnHitEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageFinished();

	/** 물리 판정 및 데미지 로직 통합 처리 함수 */
	void ProcessPhysicalHit();

	UPROPERTY(EditAnywhere, Category = "Ultimate | Animation")
	TObjectPtr<UAnimMontage> UltimateMontage;

	/** 물리 판정 범위 설정 */
	UPROPERTY(EditAnywhere, Category = "Ultimate | Collision")
	float AttackRange = 250.f;

	UPROPERTY(EditAnywhere, Category = "Ultimate | Collision")
	float AttackBoxHalfWidth = 100.f;

	UPROPERTY(EditAnywhere, Category = "Ultimate | Collision")
	float AttackBoxHalfHeight = 100.f;

	/** 몽타주에서 쏠 이벤트 태그 */
	UPROPERTY(EditAnywhere, Category = "Ultimate | Tags")
	FGameplayTag HitEventTag;

	/** 적용할 데미지 이펙트 */
	UPROPERTY(EditAnywhere, Category = "Ultimate | Damage")
	TSubclassOf<class UGameplayEffect> DamageEffectClass;

private:
	bool bFirstHitSuccess = false;
	int32 CurrentHitCount = 0;

	UPROPERTY()
	TObjectPtr<AActor> GrabbedTarget;

	UPROPERTY()
	TObjectPtr<class ASSRPlayer> CachedPlayer;

	/** 데미지 적용 내부 함수 */
	void ApplyDamageToTarget(float Amount);
};