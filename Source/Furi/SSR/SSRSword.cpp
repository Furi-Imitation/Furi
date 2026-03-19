// Fill out your copyright notice in the Description page of Project Settings.


#include "SSRSword.h"

#include "Components/BoxComponent.h"


// Sets default values
ASSRSword::ASSRSword()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// 충돌 박스 생성
	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	RootComponent = BoxCollision;
	
	// 칼 메쉬 생성
	SwordMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordMesh"));
	SwordMesh->SetupAttachment(BoxCollision);
	ConstructorHelpers::FObjectFinder<UStaticMesh> TempSwordMesh(TEXT("/Script/Engine.StaticMesh'/Game/SSR/Assets/Sword/SSRsword/StaticMeshes/sword.sword'"));
	if (TempSwordMesh.Succeeded())
	{
		SwordMesh->SetStaticMesh(TempSwordMesh.Object);
		SwordMesh->SetRelativeScale3D(FVector(0.1f));
	}
	
	// 충돌 설정
	BoxCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	BoxCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
}

// Called when the game starts or when spawned
void ASSRSword::BeginPlay()
{
	Super::BeginPlay();
	
	
}

// Called every frame
void ASSRSword::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

