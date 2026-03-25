// Fill out your copyright notice in the Description page of Project Settings.


#include "SSRAttack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h" // 실제 경로에 맞게 수정
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Kismet/GameplayStatics.h"

USSRAttack::USSRAttack()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    // 네트워크 예측 설정 (로컬에서 즉시 반응)
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    
    // 공격 중에는 이동 방지 등의 태그 설정 가능
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Lock")));
}

void USSRAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    // 로그 1: 어빌리티 시작 여부 확인
    UE_LOG(LogTemp, Warning, TEXT("SSRAttack: ActivateAbility Called!"));
    

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    CurrentComboIndex = 1;
    bComboWindowOpened = false;
    bNextComboReserved = false;
    
    // 태스크 생성 로그들
    UE_LOG(LogTemp, Log, TEXT("SSRAttack: Setting up WaitGameplayEvent Tasks..."));
    

    // 콤보 시작/종료/히트체크 이벤트 대기 태스크들
    UAbilityTask_WaitGameplayEvent* WaitStartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(FName("Combo.Start")));
    WaitStartTask->EventReceived.AddDynamic(this, &USSRAttack::OnComboEventReceived);
    WaitStartTask->ReadyForActivation();

    UAbilityTask_WaitGameplayEvent* WaitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(FName("Combo.End")));
    WaitEndTask->EventReceived.AddDynamic(this, &USSRAttack::OnComboEventReceived);
    WaitEndTask->ReadyForActivation();
    
    UAbilityTask_WaitGameplayEvent* HitCheckTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitCheckEventTag);
    HitCheckTask->EventReceived.AddDynamic(this, &USSRAttack::OnComboEventReceived);
    HitCheckTask->ReadyForActivation();
    
    if (AActor* MyAvatar = GetAvatarActorFromActorInfo())
    {
        RotateTowardsClosestEnemy(MyAvatar, AttackRange);
    }
    
    PlayComboSection();
}

void USSRAttack::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
    Super::InputPressed(Handle, ActorInfo, ActivationInfo);

    // 이미 다음 공격을 예약했다면, 더 이상의 클릭은 무시합니다.
    if (bNextComboReserved) 
    {
        UE_LOG(LogTemp, Log, TEXT("Input ignored: Combo already reserved."));
        return; 
    }

    // 윈도우가 열려있을 때만 처리 (선입력 버퍼가 없다면 이 조건이 필수)
    if (bComboWindowOpened && CurrentComboIndex < 4)
    {
        bNextComboReserved = true; 

        // 섹션 이름 조립
        FName CurrentSection = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex);
        FName NextSection = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex + 1);
        
        // 예약!
        MontageSetNextSectionName(CurrentSection, NextSection);
        
        // 예약 성공 시에만 인덱스 증가
        CurrentComboIndex++; 
        UE_LOG(LogTemp, Warning, TEXT("Combo Reserved Success! CurrentIndex is now: %d"), CurrentComboIndex);
    }
}

void USSRAttack::OnComboEventReceived(FGameplayEventData Payload)
{
    FGameplayTag EventTag = Payload.EventTag;

    if (EventTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Combo.Start"))))
    {
        bComboWindowOpened = true;
        bNextComboReserved = false; // 새 섹션 시작 시 예약 상태 초기화
    }
    else if (EventTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Combo.End"))))
    {
        bComboWindowOpened = false;
    }
    else if (EventTag.MatchesTag(HitCheckEventTag))
    {
        PerformHitCheck();
    }
}

void USSRAttack::PlayComboSection()
{
    if (!ComboMontage)
    {
        UE_LOG(LogTemp, Error, TEXT("SSRAttack: ComboMontage is NULL! Assign it in Blueprint."));
        return;
    }

    FName SectionName = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex);
    UE_LOG(LogTemp, Log, TEXT("SSRAttack: Trying to play Montage Section: %s"), *SectionName.ToString());
    
    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("AttackTask"), ComboMontage, 1.0f, SectionName);
    if (MontageTask)
    {
        MontageTask->OnCompleted.AddDynamic(this, &USSRAttack::OnMontageCompleted);
        MontageTask->OnInterrupted.AddDynamic(this, &USSRAttack::OnMontageCompleted);
        MontageTask->ReadyForActivation();
        UE_LOG(LogTemp, Log, TEXT("SSRAttack: MontageTask Activated successfully."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SSRAttack: Failed to create MontageTask!"));
    }
}

void USSRAttack::PerformHitCheck()
{
    AActor* MyAvatar = GetAvatarActorFromActorInfo();
    AGasCharacterBase* MyCharacter = Cast<AGasCharacterBase>(MyAvatar);
    if (!MyCharacter) return;

    // 현재 실제 재생 중인 섹션 확인 (데이터 동기화)
    int32 ActualIndex = CurrentComboIndex;
    UAnimInstance* AnimInstance = MyCharacter->GetMesh()->GetAnimInstance();
    if (AnimInstance)
    {
        FName CurrentSection = AnimInstance->Montage_GetCurrentSection(ComboMontage);
        FString SectionStr = CurrentSection.ToString();
        if (SectionStr.Contains(TEXT("1"))) ActualIndex = 1;
        else if (SectionStr.Contains(TEXT("2"))) ActualIndex = 2;
        else if (SectionStr.Contains(TEXT("3"))) ActualIndex = 3;
        else if (SectionStr.Contains(TEXT("4"))) ActualIndex = 4;
    }

    // --- 박스 기반 물리 판정 ---
    FVector Forward = MyAvatar->GetActorForwardVector();
    FVector BoxHalfExtent = FVector(AttackRange / 2.0f, AttackBoxHalfWidth, AttackBoxHalfHeight); 
    FVector BoxCenter = MyAvatar->GetActorLocation() + (Forward * (AttackRange / 2.0f));

    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(MyAvatar);
    
    bool bHit = GetWorld()->OverlapMultiByChannel(Overlaps, BoxCenter, MyAvatar->GetActorRotation().Quaternion(), ECC_Pawn, FCollisionShape::MakeBox(BoxHalfExtent), Params);

    if (bHit)
    {
        for (auto& Result : Overlaps)
        {
            if (AGasCharacterBase* Target = Cast<AGasCharacterBase>(Result.GetActor()))
            {
                FFuriDamageInfo DamageInfo;
                DamageInfo.Amount = (ActualIndex == 4) ? -30.f : -10.f;
                DamageInfo.DamageResponse = (ActualIndex == 4) ? EFuriDamageResponse::KnockBack : EFuriDamageResponse::HitReaction;
                DamageInfo.bCanBeParried = (ActualIndex < 4);
                DamageInfo.bCanBeBlocked = true;

                TSubclassOf<UGameplayEffect> DamageGE = ComboDamageMap.Contains(ActualIndex) ? ComboDamageMap[ActualIndex] : nullptr;
                if (DamageGE)
                {
                    FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageGE);
                    Spec.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")), DamageInfo.Amount);
                    Target->TakeFuriDamage(DamageInfo, Spec, MyAvatar);
                }
            }
        }
    }
}


void USSRAttack::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USSRAttack::RotateTowardsClosestEnemy(AActor* MyAvatar, float SearchRadius)
{
}
