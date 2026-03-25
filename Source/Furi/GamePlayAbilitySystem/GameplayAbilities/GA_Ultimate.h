#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Ultimate.generated.h"

/**
 * 돌진 후 1타 적중 시 시네마틱 연출과 함께 난무를 펼치는 궁극기 어빌리티
 */
UCLASS()
class FURI_API UGA_Ultimate : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Ultimate();

	// 어빌리티 실행 및 종료 오버라이드
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 애니메이션 노티파이(HitCheck) 이벤트를 수신했을 때 호출되는 함수 */
	UFUNCTION()
	void OnHitEventReceived(FGameplayEventData Payload);

	/** 몽타주 재생이 완료되거나 중단되었을 때 호출되는 함수 */
	UFUNCTION()
	void OnMontageFinished();

	/** 실제 대미지와 Gameplay Cue를 적용하는 내부 헬퍼 함수 */
	void ApplyUltimateDamage(FGameplayEventData Payload);

protected:
	// --- 에디터 설정 변수 ---

	UPROPERTY(EditAnywhere, Category = "Ultimate | Animation")
	TObjectPtr<UAnimMontage> UltimateMontage;

	UPROPERTY(EditAnywhere, Category = "Ultimate | Config")
	float DashForce = 2000.f;

	/** 몽타주에서 보낼 타격 이벤트 태그 (예: Event.Montage.Hit) */
	UPROPERTY(EditAnywhere, Category = "Ultimate | Tags")
	FGameplayTag HitEventTag;

	/** 적에게 적용할 대미지 Gameplay Effect 클래스 */
	UPROPERTY(EditAnywhere, Category = "Ultimate | Damage")
	TSubclassOf<class UGameplayEffect> DamageEffectClass;

private:
	// --- 내부 상태 관리 변수 ---

	/** 1타 적중 여부 (시네마틱 카메라 트리거용) */
	bool bFirstHitSuccess = false;

	/** 현재 몇 번째 타격인지 카운트 (1~3타 차등 대미지용) */
	int32 CurrentHitCount = 0;
};