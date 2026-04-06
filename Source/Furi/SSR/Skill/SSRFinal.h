#pragma once

#include "CoreMinimal.h"
#include "NiagaraFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Furi/GamePlayAbilitySystem/GameplayAbilities/FuriGameplayAbilityBase.h"
#include "Furi/utils/FuriTypes.h"
#include "SSRFinal.generated.h"

UCLASS()
class FURI_API USSRFinal : public UFuriGameplayAbilityBase
{
	GENERATED_BODY()

public:
	USSRFinal();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

protected:
	UFUNCTION()
	void OnHitEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageFinished();

	void ProcessPhysicalHit();

	// 복잡한 Map 대신, 단 하나의 대미지 전용 공용 GE를 사용합니다. (Set by Caller 연동용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Damage")
	TSubclassOf<class UGameplayEffect> BaseDamageEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate | Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> UltimateMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate | Effects", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> GrabEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate | Effects", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> HitComboEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate | Collision", meta = (AllowPrivateAccess = "true"))
	float AttackRange = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate | Collision", meta = (AllowPrivateAccess = "true"))
	float AttackBoxHalfWidth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate | Collision", meta = (AllowPrivateAccess = "true"))
	float AttackBoxHalfHeight = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate | Tags", meta = (AllowPrivateAccess = "true"))
	FGameplayTag HitEventTag;

	// 🌟 [삭제됨] DamageEffectClass 제거 (부모 클래스의 BaseDamageEffectClass로 대체)

private:
	bool bFirstHitSuccess = false;
	int32 CurrentHitCount = 0;

	UPROPERTY()
	TObjectPtr<AActor> GrabbedTarget = nullptr;

	UPROPERTY()
	TObjectPtr<class ASSRPlayer> CachedPlayer = nullptr;

	// 🌟 [수정] 단순 float 대신 FFuriDamageInfo 구조체를 받도록 수정
	void ApplyDamageToTarget(const FFuriDamageInfo& DamageInfo);
};
