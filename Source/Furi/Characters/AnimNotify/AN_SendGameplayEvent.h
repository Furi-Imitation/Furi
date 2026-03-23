#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AN_SendGameplayEvent.generated.h"

UCLASS()
class FURI_API UAN_SendGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	// 🌟 몽타주 에디터에서 태그를 직접 고를 수 있게 열어줍니다.
	UPROPERTY(EditAnywhere, Category = "GAS")
	FGameplayTag EventTag;

	// 단발성 노티파이는 Begin/End가 아니라 이거 하나만 오버라이드합니다.
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
