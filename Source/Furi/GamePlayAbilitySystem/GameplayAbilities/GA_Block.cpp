#include "GA_Block.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectRemoved.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_Block::UGA_Block()
{
    // 캐릭터마다 개별 상태를 가져야 하므로 인스턴싱 정책 설정 (멀티플레이 필수)
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    
    // 이 스킬이 멀티플레이에서 어떻게 실행될지 결정하는 아주 중요한 옵션입니다!
    // LocalPredicted: 클라이언트는 핑(Ping)을 기다리지 않고 즉시 실행하고, 서버가 나중에 승인합니다. (반응성 최고)
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Action.Block")));
}

void UGA_Block::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    // 멀티플레이 동기화의 핵심: CommitAbility를 통해 스태미나 소모 및 권한을 확인합니다.
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // 1. 방어 상태(GE) 적용
    if (BlockEffectClass)
    {
        FGameplayEffectContextHandle ContextHandle = ActorInfo->AbilitySystemComponent->MakeEffectContext();
        ContextHandle.AddInstigator(ActorInfo->AvatarActor.Get(), ActorInfo->AvatarActor.Get());
        
        FGameplayEffectSpecHandle SpecHandle = ActorInfo->AbilitySystemComponent->MakeOutgoingSpec(BlockEffectClass, 1.0f, ContextHandle);
        if (SpecHandle.IsValid())
        {
            ActiveBlockEffectHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
        }
    }

    // 2. 방어 시작 Gameplay Cue (이펙트 & 사운드)
    // 네트워크로 연결된 다른 사람들의 컴퓨터에서도 이 이펙트가 자동으로 터집니다.
    if (BlockStartCueTag.IsValid())
    {
        FGameplayCueParameters Params;
        Params.Location = ActorInfo->AvatarActor->GetActorLocation();
        Params.Instigator = ActorInfo->AvatarActor.Get();
        ActorInfo->AbilitySystemComponent->ExecuteGameplayCue(BlockStartCueTag, Params);
    }

    // 3. 방어 애니메이션 재생
    if (BlockMontage)
    {
        UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("BlockAni"), BlockMontage);
        if (MontageTask) MontageTask->ReadyForActivation();
    }

    // 4. GE가 만료되어 지워질 때까지 기다리는 요원 투입
    UAbilityTask_WaitGameplayEffectRemoved* WaitEffectTask = UAbilityTask_WaitGameplayEffectRemoved::WaitForGameplayEffectRemoved(this, ActiveBlockEffectHandle);
    if (WaitEffectTask)
    {
        // GE가 수명을 다해 사라지면 OnBlockEffectRemoved 함수 실행
        WaitEffectTask->OnRemoved.AddDynamic(this, &UGA_Block::OnBlockEffectRemoved);
        WaitEffectTask->ReadyForActivation();
    }
}

void UGA_Block::OnBlockEffectRemoved(const FGameplayEffectRemovalInfo& InGameplayEffectRemovalInfo)
{
    // 방어 이펙트 수명이 끝났으므로 스킬도 함께 종료합니다.
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Block::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    {
        //기본 모션으로 초기화
        if (BlockMontage)
        {
            // 0.25초 동안 부드럽게 기본 자세로 돌아가도록(Blend Out) 정지시킵니다.
            ActorInfo->AbilitySystemComponent->CurrentMontageStop(0.25f);
        }
        
        // 방어 종료 Gameplay Cue (방패 깨지는 소리 등)
        // bWasCancelled가 true면 적에게 맞아서 깨진 것, false면 1초가 지나서 자연스럽게 끝난 것입니다.
        if (BlockEndCueTag.IsValid())
        {
            FGameplayCueParameters Params;
            Params.Location = ActorInfo->AvatarActor->GetActorLocation();
            Params.Instigator = ActorInfo->AvatarActor.Get();
            ActorInfo->AbilitySystemComponent->ExecuteGameplayCue(BlockEndCueTag, Params);
        }

        // 영수증(Handle)을 확인해서 무적/방어 태그 떼어내기
        if (ActiveBlockEffectHandle.IsValid())
        {
            ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveBlockEffectHandle);
        }

        // 애니메이션이 아직 재생 중이라면 정지시키기
        if (BlockMontage)
        {
            ActorInfo->AbilitySystemComponent->CurrentMontageStop();
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}