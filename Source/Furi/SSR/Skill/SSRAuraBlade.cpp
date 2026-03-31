#include "SSRAuraBlade.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Furi/SSR/AuraBladeProjectile.h"
#include "GameFramework/Character.h"

USSRAuraBlade::USSRAuraBlade()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    
    // 시전 중에는 다른 행동을 제약하기 위해 태그 추가
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.SkillUsing")));
}

void USSRAuraBlade::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    // 1. 자원 및 쿨타임 체크
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
       EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
       return;
    }
    
    if (ChargeMontage)
    {
        // 몽타주를 재생하고 기다리는 태스크
        UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ChargeMontage);
        MontageTask->ReadyForActivation();
        UE_LOG(LogTemp, Log, TEXT("[AuraBlade] 몽타주 재생 시작"));
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC) return;

    UE_LOG(LogTemp, Warning, TEXT("[AuraBlade] 시전 시작! %f초 대기 중..."), DelayTime);

    // 2. 기 모으기 비주얼 효과 시작 (GameplayCue)
    if (ChargeCueTag.IsValid())
    {
        ASC->AddGameplayCue(ChargeCueTag);
    }

    // 3. [핵심] WaitDelay 태스크 생성 (지정한 시간만큼 대기)
    UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, DelayTime);
    if (DelayTask)
    {
        // 시간이 다 되면 OnDelayFinished 함수를 호출하도록 연결
        DelayTask->OnFinish.AddDynamic(this, &USSRAuraBlade::OnDelayFinished);
        DelayTask->ReadyForActivation();
    }
    else
    {
        // 태스크 생성 실패 시 즉시 종료
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    }
}

void USSRAuraBlade::OnDelayFinished()
{
    UE_LOG(LogTemp, Warning, TEXT("[AuraBlade] 대기 완료! 발사!"));

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    
    if (!Character || !ASC)
    {
       EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
       return;
    }

    // 1. 기 모으기 효과 제거 및 발사 효과 실행
    if (ChargeCueTag.IsValid()) { ASC->RemoveGameplayCue(ChargeCueTag); }
    if (FireCueTag.IsValid()) { ASC->ExecuteGameplayCue(FireCueTag); }

    // 2. 데미지 설정
    FGameplayEffectSpecHandle DamageSpecHandle;
    if (DamageEffectClass)
    {
        DamageSpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
        if (DamageSpecHandle.IsValid())
        {
            FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"));
            UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageSpecHandle, DamageTag, AbilityDamage);
        }
    }

    // 3. 투사체 소환 (정면 1m 지점)
    if (ProjectileClass)
    {
       FVector ForwardVector = Character->GetActorForwardVector();
       FVector SpawnLocation = Character->GetActorLocation() + (ForwardVector * 100.f) + FVector(0.f, 0.f, 50.f);
       FRotator SpawnRotation = Character->GetActorRotation();
    
       FActorSpawnParameters SpawnParams;
       SpawnParams.Owner = Character;
       SpawnParams.Instigator = Character;
       SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
       AAuraBladeProjectile* Projectile = GetWorld()->SpawnActor<AAuraBladeProjectile>(
          ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
       
       if (Projectile)
       {
          Projectile->Initialize(AbilityDamage, 1.0f, DamageSpecHandle);
          UE_LOG(LogTemp, Warning, TEXT("[AuraBlade] 투사체 생성 성공!"));
       }
    }

    // 4. 모든 시퀀스 종료
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

