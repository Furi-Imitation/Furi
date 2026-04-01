#include "FuriGameplayAbilityBase.h"

#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"

bool UFuriGameplayAbilityBase::GetCurrentSkillData(FFuriSkillData& OutData) const
{
    if (AGasCharacterBase* MyChar = Cast<AGasCharacterBase>(GetAvatarActorFromActorInfo()))
    {
        if (UFuriSkillDataAsset* DA = MyChar->GetSkillUIData())
        {
            // 이 어빌리티에 설정된 첫 번째 태그(예: Ability.Action.Dash)를 키값으로 데이터를 찾습니다.
            if (AbilityTags.IsValid() && DA->GetSkillDataByTag(AbilityTags.First(), OutData))
            {
                return true;
            }
        }
    }
    return false;
}

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