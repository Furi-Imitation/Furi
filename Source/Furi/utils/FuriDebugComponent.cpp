#include "FuriDebugComponent.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Furi/GamePlayAbilitySystem/Characters/GasCharacterBase.h"
#include "Furi/GamePlayAbilitySystem/GameplayAbilities/GA_Attack.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

UFuriDebugComponent::UFuriDebugComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFuriDebugComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AGasCharacterBase* Owner = Cast<AGasCharacterBase>(GetOwner()))
	{
		ASC = Owner->GetAbilitySystemComponent();
		if (ASC)
		{
			ASC->AbilityActivatedCallbacks.AddUObject(this, &UFuriDebugComponent::OnAbilityActivated);
			
			// 전역 태그 변경 감시 (예시 몇 개 등록)
			ASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UFuriDebugComponent::OnTagChanged);
		}
	}
}

void UFuriDebugComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 콤보 데이터 실시간 업데이트 (GA_Attack이 활성화되어 있다면)
	if (ASC)
	{
		TArray<FGameplayAbilitySpec> Specs = ASC->GetActivatableAbilities();
		for (const FGameplayAbilitySpec& Spec : Specs)
		{
			if (Spec.IsActive() && Spec.Ability->IsA<UGA_Attack>())
			{
				// GA_Attack의 내부 변수에 접근하기 위해 캐스팅 (실제로는 public getter가 필요할 수 있음)
				// 여기서는 데모를 위해 직접 접근하거나 추후 보완
			}
		}
	}
}

void UFuriDebugComponent::OnAbilityActivated(UGameplayAbility* Ability)
{
	if (!Ability)
	{
		return;
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(FuriGAS_AbilityActivated);

	FFuriPredictionDebugData Data;
	Data.Timestamp = GetWorld()->GetTimeSeconds();
	Data.AbilityName = Ability->GetName();
	
	// Prediction 상태 판단
	if (Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)
	{
		Data.Status = EFuriPredictionStatus::Predicted;
	}
	else
	{
		Data.Status = EFuriPredictionStatus::Confirmed;
	}

	// 🌟 즉각적인 로그/화면 출력 추가
	FString StatusStr = (Data.Status == EFuriPredictionStatus::Predicted) ? TEXT("PREDICTED") : TEXT("CONFIRMED");
	FColor DisplayColor = (Data.Status == EFuriPredictionStatus::Predicted) ? FColor::Yellow : FColor::Green;
	
	UE_LOG(LogTemp, Log, TEXT("[GAS-Debug] Ability: %s | Status: %s"), *Data.AbilityName, *StatusStr);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, DisplayColor, 
			FString::Printf(TEXT("[%s] %s"), *StatusStr, *Data.AbilityName));
	}

	PredictionHistory.Insert(Data, 0);
	if (PredictionHistory.Num() > MaxPredictionHistory)
	{
		PredictionHistory.RemoveAt(MaxPredictionHistory);
	}
}

void UFuriDebugComponent::OnTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 태그 리스트 업데이트
	bool bFound = false;
	for (FFuriTagDebugData& TagData : ActiveTags)
	{
		if (TagData.Tag == Tag)
		{
			TagData.Count = NewCount;
			bFound = true;
			break;
		}
	}

	if (!bFound && NewCount > 0)
	{
		FFuriTagDebugData NewTagData;
		NewTagData.Tag = Tag;
		NewTagData.Count = NewCount;
		// 태그 종류에 따라 색상 지정
		if (Tag.ToString().Contains(TEXT("Status"))) NewTagData.DisplayColor = FColor::Red;
		else if (Tag.ToString().Contains(TEXT("Ability"))) NewTagData.DisplayColor = FColor::Green;
		else NewTagData.DisplayColor = FColor::White;

		ActiveTags.Add(NewTagData);
		
		// 🌟 태그 획득 시 로그 출력
		UE_LOG(LogTemp, Warning, TEXT("[GAS-Debug] Tag Added: %s (Count: %d)"), *Tag.ToString(), NewCount);
	}

	// 카운트가 0이면 제거
	ActiveTags.RemoveAll([](const FFuriTagDebugData& TD) { return TD.Count <= 0; });
}

void UFuriDebugComponent::UpdateNetSerializeData(int32 Raw, int32 Optimized)
{
	NetSerializeData.RawBytes = Raw;
	NetSerializeData.OptimizedBytes = Optimized;
	if (Raw > 0)
	{
		NetSerializeData.ReductionPercentage = (1.0f - (float)Optimized / (float)Raw) * 100.0f;
	}

	// 🌟 네트워크 최적화 결과 로그/화면 출력
	UE_LOG(LogTemp, Warning, TEXT("[GAS-Debug] NetSerialize Optimization: %d -> %d bytes (Saved %.1f%%)"), 
		Raw, Optimized, NetSerializeData.ReductionPercentage);
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(100, 1.0f, FColor::Cyan, 
			FString::Printf(TEXT("Net Pack: %d -> %d bytes (Saved %.1f%%)"), Raw, Optimized, NetSerializeData.ReductionPercentage));
	}
}

void UFuriDebugComponent::UpdateComboData(int32 Index, bool bOpened, bool bReserved, FString FailureReason)
{
	ComboData.CurrentComboIndex = Index;
	ComboData.bWindowOpened = bOpened;
	ComboData.bReserved = bReserved;
	ComboData.LastFailureReason = FailureReason;

	// 🌟 콤보 상태 실시간 로그
	if (!FailureReason.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[GAS-Debug] Combo Failed: %s"), *FailureReason);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, FString::Printf(TEXT("Combo Fail: %s"), *FailureReason));
	}
	else if (bOpened)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(101, 0.5f, FColor::Orange, TEXT("Combo Window: OPEN"));
	}
}
