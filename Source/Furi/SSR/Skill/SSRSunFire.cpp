// Fill out your copyright notice in the Description page of Project Settings.


#include "SSRSunFire.h"

#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "AbilitySystemGlobals.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Furi/GamePlayAbilitySystem/AttributeSets/BasicAttributeSet.h"
#include "GameFramework/Actor.h"


USSRSunFire::USSRSunFire()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Action.SunFire")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.SunFire")));
}

void USSRSunFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	
	// 이미 켜져있으면 끄기
	if (bIsActive)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	
	// 코스트 or 쿨타임 체크
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	
	// 멀티플레이용
	// if (!HasAuthority(&CurrentActivationInfo))
	// {
	// 	return;
	// }
	
	bIsActive = true;
	
	// 캐릭터 가져오기
	AActor* OwnerActor = GetAvatarActorFromActorInfo();
	if (!OwnerActor)
	{
		return;
	}
	
	// 시작 Cue
	if (SunFireStartCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = OwnerActor->GetActorLocation();
		Params.Instigator = OwnerActor;
		
		GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(SunFireStartCueTag, Params);
	}
	
	ApplySunFireDamage();
	// // 진행중 -> 반복 데미지
	// GetWorld()->GetTimerManager().SetTimer(
	// 	TickTimerHandle,
	// 	this,
	// 	&USSRSunFire::ApplySunFireDamage,
	// 	TickInterval,
	// 	true
	// 	);
}

void USSRSunFire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bwasCancelled)
{
	
	AActor* OwnerActor = GetAvatarActorFromActorInfo();
	
	// 타이머 종료
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TickTimerHandle);
	}
	
	// 종료 Cue
	bIsActive = false;
	if (OwnerActor && SunFireEndCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = OwnerActor->GetActorLocation();
		Params.Instigator = OwnerActor;
		
		GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(SunFireEndCueTag, Params);
	}
	
	// EndAbility가 끝나고 마지막에 호출하여 Ability 완전히 종료
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bwasCancelled);
}

void USSRSunFire::ApplySunFireDamage()
{
	AActor* OwnerActor = GetAvatarActorFromActorInfo();
	if (!OwnerActor)
		return;
	
	TArray<AActor*> Actors;
	GetActorInRange(Actors);
	
	for (AActor* Target : Actors)
	{
		if (!Target || Target == OwnerActor)
			continue;
		
		UAbilitySystemComponent* TargetASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
		
		if (TargetASC && DamageEffectClass)
		{
			FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
			Context.AddSourceObject(this);
			
			FGameplayEffectSpecHandle SpecHandle =
				TargetASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
			
			if (SpecHandle.IsValid())
			{
				TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
	
	// 진행 Cue
	if (SunFireLoopCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = OwnerActor->GetActorLocation();
		Params.Instigator = OwnerActor;
		
		GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(SunFireLoopCueTag, Params);
	}
}

// SunFire 공격 범위
void USSRSunFire::GetActorInRange(TArray<AActor*>& OutActors)
{
	
	AActor* OwnerActor = GetAvatarActorFromActorInfo();
	if (!OwnerActor)
		return;
	
	FVector Center = OwnerActor->GetActorLocation();
	
	TArray<FOverlapResult> Overlaps;
	
	FCollisionQueryParams Params;
	// 본인 무시
	Params.AddIgnoredActor(OwnerActor);
	
	UWorld* World = GetWorld();
	if (!World) return;
	
	World->OverlapMultiByChannel(
		Overlaps,
		Center,
		FQuat::Identity, // 아무것도 회전하지 않은 기본값
		ECC_Pawn,
		FCollisionShape::MakeSphere(Radius),
		Params
		);
	UE_LOG(LogTemp, Warning, TEXT("1235463215675372461"));
	DrawDebugSphere(GetWorld(), OwnerActor->GetActorLocation(), Radius, 32, FColor::Red,false, 5.0f);
	for (const FOverlapResult& Result : Overlaps)
	{
		if (AActor* HitActor = Result.GetActor())
		{
			OutActors.Add(HitActor);
		}
	}
}

