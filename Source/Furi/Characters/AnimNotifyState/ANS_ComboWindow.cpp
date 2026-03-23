#include "ANS_ComboWindow.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

void UANS_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AActor* Owner = MeshComp->GetOwner())
	{
		// 🌟 프리뷰 액터가 아닌, 실제 GAS를 가진 녀석인지 검사
		if (UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner))
		{
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, FGameplayTag::RequestGameplayTag(FName("Combo.Start")), FGameplayEventData());
		}
	}
}

void UANS_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AActor* Owner = MeshComp->GetOwner())
	{
		// 🌟 여기도 똑같이 검사
		if (UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner))
		{
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, FGameplayTag::RequestGameplayTag(FName("Combo.End")), FGameplayEventData());
		}
	}
}