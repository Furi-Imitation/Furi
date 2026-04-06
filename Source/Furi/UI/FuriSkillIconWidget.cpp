#include "FuriSkillIconWidget.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "AbilitySystemComponent.h"
#include "Furi/utils/FuriSkillDataAsset.h"

void UFuriSkillIconWidget::InitSkillWidget(const FFuriSkillData& SkillData, UAbilitySystemComponent* InASC)
{
	ASC = InASC;
	CooldownTag = SkillData.CooldownTag;

	if (Text_StaminaCost)
	{
		Text_StaminaCost->SetText(FText::AsNumber(FMath::RoundToInt(SkillData.StaminaCost)));
	}

	if (SkillData.SkillIcon)
	{
		// 1. 어두운 배경 아이콘 설정
		if (Image_BackgroundIcon)
		{
			Image_BackgroundIcon->SetBrushFromTexture(SkillData.SkillIcon);
			Image_BackgroundIcon->SetColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f, 1.0f));
		}

		// 2. 🌟 차오르는 프로그레스 바 이미지 렌더링 규칙 수정
		if (PB_CooldownOverlay)
		{
			FProgressBarStyle Style = PB_CooldownOverlay->GetWidgetStyle();

			// Fill Image 세팅
			Style.FillImage.SetResourceObject(SkillData.SkillIcon);
			// 🌟 [핵심 1] 박스 형태가 아니라 순수 이미지로 그리도록 강제 설정
			Style.FillImage.DrawAs = ESlateBrushDrawType::Image;

			// 🌟 [핵심 2] 프로그레스 바 자체의 회색 배경이 아이콘을 가리지 않게 아예 꺼버림
			Style.BackgroundImage.DrawAs = ESlateBrushDrawType::NoDrawType;

			PB_CooldownOverlay->SetWidgetStyle(Style);
		}
	}
}

void UFuriSkillIconWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!ASC.IsValid() || !CooldownTag.IsValid() || !PB_CooldownOverlay)
	{
		return;
	}

	// 🌟 1. 이 스킬의 쿨타임 태그를 가지고 있는 모든 이펙트를 찾아내는 '검색 조건(Query)'을 만듭니다.
	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(CooldownTag));

	// 🌟 2. 해당 조건에 맞는 이펙트들의 전체 시간(Duration)과 남은 시간(Remaining)을 리스트로 뽑아옵니다.
	TArray<float> Durations = ASC->GetActiveEffectsDuration(Query);
	TArray<float> Remainings = ASC->GetActiveEffectsTimeRemaining(Query);

	float MaxTimeRemaining = 0.f;
	float MaxTotalDuration = 0.f;

	// 🌟 3. 혹시라도 쿨타임이 여러 개 겹쳤을 경우를 대비해, 가장 길게 남은 진짜 쿨타임을 찾습니다.
	if (Durations.Num() > 0 && Remainings.Num() > 0)
	{
		for (int32 i = 0; i < Remainings.Num(); ++i)
		{
			if (Remainings[i] > MaxTimeRemaining)
			{
				MaxTimeRemaining = Remainings[i];
				MaxTotalDuration = Durations[i];
			}
		}
	}

	// 🌟 4. 찾아낸 쿨타임을 바탕으로 UI 게이지를 갱신합니다.
	if (MaxTotalDuration > 0.f && MaxTimeRemaining > 0.f)
	{
		// 쿨타임 중: 시간이 지날수록 (0.0 -> 1.0) 으로 차오르게 계산합니다.
		float Percent = (MaxTotalDuration - MaxTimeRemaining) / MaxTotalDuration;
		PB_CooldownOverlay->SetPercent(Percent);
	}
	else
	{
		// 쿨타임 아님: 100% 꽉 찬 상태(밝은 원본 아이콘 전체 표시)
		PB_CooldownOverlay->SetPercent(1.0f);
	}
}
