#include "FuriGameplayAbilityBase.h"

#include "Furi/GamePlayAbilitySystem/AttributeSets/BasicAttributeSet.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"

void UFuriGameplayAbilityBase::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
    UGameplayEffect* CostGE = GetCostGameplayEffect();
    if (CostGE)
    {
        FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CostGE->GetClass(), GetAbilityLevel());

        // 🌟 Data Asset에서 스태미나 소모량을 가져와 SetByCaller로 주입합니다!
        FFuriSkillData SkillData;
        if (GetCurrentSkillData(SkillData) && SpecHandle.IsValid())
        {
            // 빼야 하므로 음수(-)로 변환해서 넣습니다.
            SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Cost.Stamina")), -SkillData.StaminaCost);
        }

        (void)ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
    }
}

void UFuriGameplayAbilityBase::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
    UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
    if (CooldownGE)
    {
        FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGE->GetClass(), GetAbilityLevel());

        // 🌟 Data Asset에서 쿨타임 시간을 가져와 SetByCaller로 주입합니다!
        FFuriSkillData SkillData;
        if (GetCurrentSkillData(SkillData) && SpecHandle.IsValid())
        {
            SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Cooldown.Duration")), SkillData.CooldownDuration);
        }

       (void)ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
    }
}

bool UFuriGameplayAbilityBase::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    // 1. Data Asset에서 현재 스킬의 스태미나 소모량을 가져옵니다.
    FFuriSkillData SkillData;
    if (GetCurrentSkillData(SkillData) && SkillData.StaminaCost > 0.f)
    {
        UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
        if (ASC)
        {
            // 2. 플레이어의 현재 스태미나 수치를 직접 읽어옵니다.
            float CurrentStamina = ASC->GetNumericAttribute(UBasicAttributeSet::GetStaminaAttribute());
            
            // 3. 스태미나가 스킬 코스트보다 부족하다면 결제 거부! (false)
            if (CurrentStamina < SkillData.StaminaCost)
            {
                if (OptionalRelevantTags)
                {
                    if (UGameplayEffect* CostGE = GetCostGameplayEffect())
                    {
                        // UI에 '스태미나 부족'을 알리기 위해 태그를 전달합니다.
                        OptionalRelevantTags->AppendTags(CostGE->GetAssetTags());
                    }
                }
                return false; 
            }
        }
    }
    
    // 코스트가 0이거나 스태미나가 충분하면 결제 승인! (true)
    return true;
}

bool UFuriGameplayAbilityBase::GetCurrentSkillData(FFuriSkillData& OutData, const FGameplayAbilityActorInfo* InActorInfo) const
{
    // 🌟 핵심: 파라미터로 정보가 들어왔으면 그걸 쓰고, 아니면 내 정보를 씁니다.
    const FGameplayAbilityActorInfo* TargetActorInfo = InActorInfo ? InActorInfo : CurrentActorInfo;
    
    // 타겟 정보나 아바타 액터가 비어있으면 안전하게 빠져나갑니다 (크래시 방지)
    if (!TargetActorInfo || !TargetActorInfo->AvatarActor.IsValid())
    {
        return false;
    }

    if (AGasCharacterBase* MyChar = Cast<AGasCharacterBase>(TargetActorInfo->AvatarActor.Get()))
    {
        if (UFuriSkillDataAsset* DA = MyChar->GetSkillUIData())
        {
            if (AbilityTags.IsValid() && DA->GetSkillDataByTag(AbilityTags.First(), OutData))
            {
                return true;
            }
        }
    }
    return false;
}