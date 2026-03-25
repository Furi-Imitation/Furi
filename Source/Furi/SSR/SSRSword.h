// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SSRSword.generated.h"

UCLASS()
class FURI_API ASSRSword : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASSRSword();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
public:
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* BoxCollision;
	
	UPROPERTY(visibleAnywhere)
	class UStaticMeshComponent* SwordMesh;
	
	
	
};
