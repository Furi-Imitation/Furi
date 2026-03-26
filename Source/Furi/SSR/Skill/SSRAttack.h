#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Furi/utils/FuriTypes.h" // FFuriDamageInfo가 정의된 위치
#include "SSRAttack.generated.h"

UCLASS()
class FURI_API USSRAttack : public UGameplayAbility
{
	GENERATED_BODY()
    
public:
	USSRAttack();
    
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    
	// 사용자가 공격 키를 눌렀을 때 엔진이 호출해주는 함수
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	UFUNCTION()
	void OnComboEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	void PlayComboSection();
	void PerformHitCheck();
	void RotateTowardsClosestEnemy(AActor* MyAvatar, float SearchRadius);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	UAnimMontage* ComboMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TMap<int32, TSubclassOf<class UGameplayEffect>> ComboDamageMap;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	FGameplayTag HitCheckEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackRange = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackBoxHalfWidth = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackBoxHalfHeight = 50.0f;

	int32 CurrentComboIndex = 4;
	// 다음 콤보가 실행될수 있는 구간에 들어왔는지 체크
	bool bComboWindowOpened = false;
	// 키를 입력바아서 다음 콤보가 예약받았을때
	bool bNextComboReserved = false;

	UPROPERTY()
	class UAbilityTask_PlayMontageAndWait* MontageTask;
};