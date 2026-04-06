#pragma once

#include "CoreMinimal.h"
#include "Furi/GamePlayAbilitySystem/GameplayAbilities/FuriGameplayAbilityBase.h"
#include "Furi/utils/FuriTypes.h"
#include "SSRAttack.generated.h"

UCLASS()
class FURI_API USSRAttack : public UFuriGameplayAbilityBase
{
	GENERATED_BODY()

public:
	USSRAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	// 사용자가 공격 키를 눌렀을 때 엔진이 호출해주는 함수
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                          const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	UFUNCTION()
	void OnComboEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	void PlayComboSection();
	void PerformHitCheck();
	void RotateTowardsClosestEnemy(AActor* MyAvatar, float SearchRadius);

	// 복잡한 Map 대신, 단 하나의 대미지 전용 공용 GE를 사용합니다. (Set by Caller 연동용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Damage")
	TSubclassOf<class UGameplayEffect> BaseDamageEffectClass;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ComboMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (AllowPrivateAccess = "true"))
	FGameplayTag HitCheckEventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (AllowPrivateAccess = "true"))
	float AttackRange = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (AllowPrivateAccess = "true"))
	float AttackBoxHalfWidth = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (AllowPrivateAccess = "true"))
	float AttackBoxHalfHeight = 50.0f;

	int32 CurrentComboIndex = 4;

	// 다음 콤보가 실행될 수 있는 구간에 들어왔는지 체크
	bool bComboWindowOpened = false;

	// 키를 입력받아서 다음 콤보가 예약되었을 때
	bool bNextComboReserved = false;

	UPROPERTY()
	class UAbilityTask_PlayMontageAndWait* MontageTask = nullptr;
};
