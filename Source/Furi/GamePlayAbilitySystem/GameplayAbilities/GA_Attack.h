#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Furi/utils/FuriTypes.h" 
#include "GameplayTagContainer.h"
#include "Sound/SoundBase.h"
#include "GA_Attack.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

/**
 * 1대1 근접 콤보 공격 어빌리티 (Box Sweep 타격 판정)
 */
UCLASS()
class FURI_API UGA_Attack : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Attack();

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

    // 콤보 및 타격 이벤트를 수신하는 콜백 함수
    UFUNCTION()
    void OnComboEventReceived(FGameplayEventData Payload);

    // 몽타주 재생이 완전히 끝났을 때 호출되는 콜백 함수
    UFUNCTION()
    void OnMontageCompleted();

    // 1대1 전용 타격 판정 실행 함수
    void PerformHitCheck();

    // 첫 공격 몽타주를 실행하는 함수
    void PlayComboSection();

    // 상대방을 향해 회전
    void RotateTowardsClosestEnemy(AActor* MyActor, float SearchRadius);
protected:
    // --- [에디터 설정 변수] ---
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Animation")
    UAnimMontage* ComboMontage;

    // 타격 판정 시점을 알려주는 몽타주 노티파이 태그 (예: Event.Hit.Check)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Animation")
    FGameplayTag HitCheckEventTag;

    // 콤보 타수(1, 2, 3)별로 적용할 GameplayEffect(대미지) 맵
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Damage")
    TMap<int32, TSubclassOf<class UGameplayEffect>> ComboDamageMap;

protected:
    // --- [물리 판정 (Hit Check) 설정 변수] ---
    
    // 1. 공격 사거리 (내 중심에서 앞으로 얼마나 멀리 판정할 것인가)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitCheck")
    float AttackRange = 300.0f; 

    // 2. 상자의 좌우 너비 절반 (80.0f면 총 좌우 너비는 160cm)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitCheck")
    float AttackBoxHalfWidth = 80.0f; 

    // 3. 상자의 위아래 높이 절반 (60.0f면 총 높이는 120cm)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitCheck")
    float AttackBoxHalfHeight = 60.0f;
    
private:
    // --- [내부 상태 관리 변수] ---
    
    int32 CurrentComboIndex;       // 현재 몇 타째인지 추적 (1~3)
    bool bComboWindowOpened;       // 현재 콤보 입력이 가능한 구간인지?
    bool bNextComboReserved;       // 사용자가 클릭해서 다음 콤보가 예약되었는지?
    
    UPROPERTY()
    UAbilityTask_PlayMontageAndWait* MontageTask;
    
protected:
    // 허공 휘두르기 사운드 (에디터에서 "쉭" 소리 할당)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    TObjectPtr<USoundBase> SwingSound;

    // 허공 사운드 재생 함수
    void PlaySwingSound();
};