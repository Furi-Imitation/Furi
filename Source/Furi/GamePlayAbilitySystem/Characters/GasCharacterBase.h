// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GasCharacterBase.generated.h"

enum class EFuriDamageResponse : uint8;
class UBasicAttributeSet;

UCLASS()
class FURI_API AGasCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGasCharacterBase();

	// Ability System Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	UBasicAttributeSet* BasicAttributeSet;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability System")
	EGameplayEffectReplicationMode AscReplicationMode = EGameplayEffectReplicationMode::Mixed;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;
	
	virtual void OnRep_PlayerState() override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
public:
	// 🌟 피격 반응을 통합 처리하는 핵심 함수 (Attacker를 받아 날아갈 방향을 계산함)
	void HandleDamageResponse(EFuriDamageResponse Response, AActor* Attacker = nullptr);

protected:
	// --- [애니메이션 설정] ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	UAnimMontage* HitReactionMontage; // 가벼운 피격 (움찔)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	UAnimMontage* StaggerMontage;     // 강한 피격 (비틀거림)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	UAnimMontage* KnockBackMontage;   // 넉백 (공중에 뜸/날아감)

	// --- [넉백 물리 설정] ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Physics")
	float KnockBackPushForce = 1500.f; // 뒤로 밀어내는 힘 (수평)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Physics")
	float KnockBackUpForce = 600.f;  
	
public:
	// 공격자로부터 대미지 정보와 GAS Effect 정보를 함께 전달받는 함수
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void TakeFuriDamage(const FFuriDamageInfo& DamageInfo, const FGameplayEffectSpecHandle& DamageSpec, AActor* InstigatorActor);// 위로 띄우는 힘 (수직)
};
