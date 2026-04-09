#include "BasicAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/GamePlayAbilitySystem/FuriAbilityTypes.h"
#include "Net/UnrealNetwork.h"

UBasicAttributeSet::UBasicAttributeSet()
{
	// 캐릭터가 처음 생성될 때 가질 기본 스탯들입니다.
	Health = 1000.f;
	MaxHealth = 1000.f;
	Stamina = 100.f;
	MaxStamina = 100.f;
}

void UBasicAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
}

void UBasicAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
}

bool UBasicAttributeSet::PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		const float DamageMagnitude = Data.EvaluatedData.Magnitude;
		if (DamageMagnitude < 0.0f) // 데미지(음수 Magnitude)인 경우에만 체크
		{
			UAbilitySystemComponent* TargetASC = &Data.Target;
			FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
			FFuriGameplayEffectContext* FuriContext = FFuriGameplayEffectContext::GetFuriContext(Context);

			bool bIsInvincible = false;
			bool bIsBlocked = false;

			if (FuriContext)
			{
				const FFuriDamageInfo& DamageInfo = FuriContext->GetDamageInfo();

				// 1. 무적 체크
				if (TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Invincible"))) && !
					DamageInfo.bShouldDamageInvincible)
				{
					bIsInvincible = true;
				}

				// 2. 방어 체크
				if (!bIsInvincible && TargetASC->HasMatchingGameplayTag(
					FGameplayTag::RequestGameplayTag(FName("State.Blocking"))) && DamageInfo.bCanBeBlocked)
				{
					bIsBlocked = true;
				}
			}

			if (bIsInvincible || bIsBlocked)
			{
				if (bIsInvincible)
				{
					UE_LOG(LogTemp, Warning, TEXT("[BasicAttributeSet] PreExecute: Damage Ignored (Invincible)!"));
				}
				else if (bIsBlocked)
				{
					UE_LOG(LogTemp, Warning,
					       TEXT("[BasicAttributeSet] PreExecute: Damage Blocked (Canceling Block Skill)!"));

					FGameplayTagContainer BlockTags;
					BlockTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Blocking")));
					TargetASC->RemoveActiveEffectsWithGrantedTags(BlockTags);
				}

				return false; // 🛡️ 아예 적용되지 않도록 차단!
			}
		}
	}

	return true;
}

void UBasicAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	AActor* SourceActor = Context.GetOriginalInstigator();
	AActor* TargetActor = Data.Target.GetAvatarActor();
	AGasCharacterBase* TargetCharacter = Cast<AGasCharacterBase>(TargetActor);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));

		if (GetHealth() <= 0.0f)
		{
			if (TargetCharacter)
			{
				UE_LOG(LogTemp, Warning, TEXT("[BasicAttributeSet] %s is Dead!"), *TargetCharacter->GetName());
				TargetCharacter->Die();
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[BasicAttributeSet] Health changed. Current: %.2f"), GetHealth());

			// 데미지를 입었을 때의 리액션 처리 (커스텀 컨텍스트 활용)
			FFuriGameplayEffectContext* FuriContext = FFuriGameplayEffectContext::GetFuriContext(Context);
			if (FuriContext)
			{
				const FFuriDamageInfo& DamageInfo = FuriContext->GetDamageInfo();
				if (TargetCharacter)
				{
					UE_LOG(LogTemp, Warning, TEXT("[BasicAttributeSet] Calling HandleDamageResponse for %s"),
					       *TargetCharacter->GetName());
					TargetCharacter->HandleDamageResponse(DamageInfo, SourceActor);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[BasicAttributeSet] TargetCharacter is NULL!"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error,
				       TEXT(
					       "[BasicAttributeSet] FuriContext is NULL! Check if the GE is using FFuriGameplayEffectContext."
				       ));
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}
}
