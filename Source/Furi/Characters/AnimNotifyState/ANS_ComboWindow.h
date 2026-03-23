#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_ComboWindow.generated.h"

// 콤보 애니메이션의 특정 구간을 나타내는 애니메이션 노티파이 상태 클래스입니다.
UCLASS()
class FURI_API UANS_ComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	// 노티파이 구간이 시작될 때 호출
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	// 노티파이 구간이 끝날 때 호출
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
