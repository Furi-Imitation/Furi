// Fill out your copyright notice in the Description page of Project Settings.


#include "NeulPlayer.h"


// Sets default values
ANeulPlayer::ANeulPlayer()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANeulPlayer::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANeulPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ANeulPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

