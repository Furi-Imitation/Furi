// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h" // GAS 데미지 처리
#include "GameFramework/Actor.h"
#include "AuraBladeProjectile.generated.h"

UCLASS()
class FURI_API AAuraBladeProjectile : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AAuraBladeProjectile();
	
	UPROPERTY()
	FGameplayTag ImpactCueTag;

	void Initialize(float InDamage, float InChargeRatio, FGameplayEffectSpecHandle InSpecHandle, FGameplayTag InHitTag);
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// 무언가에 부딪혔을 때 호출될 함수
	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 충돌체
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class USphereComponent* CollisionComp;
	
	// 나이아가라 컴포넌트
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	// class UNiagaraComponent* NiagaraComp;

	// 발사체 움직임 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UProjectileMovementComponent* ProjectileMovement;
	
	
	// UPROPERTY()
	// TArray<AActor*> HitActors;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	float Damage;
	float ChargeRatio;
	FGameplayEffectSpecHandle DamageEffectSpecHandle; // GAS 데미지 데이터
};
