#include "AN_SendGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h" // 프리뷰 에러 방지용

void UAN_SendGameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AActor* Owner = MeshComp->GetOwner())
	{
		// 프리뷰 액터인지(진짜 GAS를 가졌는지) 검사해서 에러 로그 도배를 막습니다.
		if (UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner))
		{
			// 🌟 몽타주 에디터에서 설정한 바로 그 태그를 ASC로 발사합니다!
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, FGameplayEventData());
		}
	}
}