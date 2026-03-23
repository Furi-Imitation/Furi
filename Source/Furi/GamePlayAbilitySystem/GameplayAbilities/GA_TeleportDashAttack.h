#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_TeleportDashAttack.generated.h"

UCLASS()
class FURI_API UGA_TeleportDashAttack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_TeleportDashAttack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// --- 스킬 시퀀스 제어 함수 ---
	// 다음 타격을 위해 숨고 0.5초 대기하는 함수
	UFUNCTION()
	void PrepareNextStrike(); 

	// 대기가 끝나고 텔레포트 및 몽타주를 재생하는 함수
	UFUNCTION()
	void OnStrikeDelayFinished(); 

	UFUNCTION()
	void OnStrikeMontageFinished();

	UFUNCTION()
	void OnStrikeInterrupted();

	UFUNCTION()
	void OnHitCheckEventReceived(FGameplayEventData Payload);

protected:
	// 진행 상태 관리
	int32 CurrentStrikeCount;
	const int32 MaxStrikeCount = 3;

	UPROPERTY()
	AActor* LockedTarget;

	// 각 타수별 애니메이션 몽타주 (크기가 3인 배열로 에디터에서 세팅)
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<TObjectPtr<UAnimMontage>> StrikeMontages;

	// 타격 판정을 트리거할 이벤트 태그 (예: Event.Melee.HitCheck)
	UPROPERTY(EditDefaultsOnly, Category = "Tags")
	FGameplayTag HitEventTag;

	// 비주얼 연출을 위한 큐 태그 (은신, 잔상 등)
	UPROPERTY(EditDefaultsOnly, Category = "Tags")
	FGameplayTag VanishCueTag;
	
	UPROPERTY(EditDefaultsOnly, Category = "Tags")
	TSubclassOf<class UGameplayEffect> DamageGEClass;
	
protected:
	
	UPROPERTY(EditDefaultsOnly, Category = "Teleport")
	float StrikeDelay = 0.5f;
	
protected:
	// --- 타격 판정 수치 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitCheck")
	float AttackRange = 300.0f; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitCheck")
	float AttackBoxHalfWidth = 80.0f; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitCheck")
	float AttackBoxHalfHeight = 60.0f;
};