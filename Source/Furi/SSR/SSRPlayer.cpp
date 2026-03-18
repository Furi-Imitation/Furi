// Fill out your copyright notice in the Description page of Project Settings.


#include "SSRPlayer.h"

#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GameFramework/CharacterMovementComponent.h"

ASSRPlayer::ASSRPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	
	// 검 구현
	SwordMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>("SwordMeshComp");
	SwordMeshComp->SetupAttachment(GetMesh());
	ConstructorHelpers::FObjectFinder<USkeletalMesh> TempSwordMesh(TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
	if (TempSwordMesh.Succeeded())
	{
		SwordMeshComp->SetSkeletalMesh(TempSwordMesh.Object);
		SwordMeshComp->SetRelativeLocation(FVector(-0.000000,44.444444,55.555556));
		SwordMeshComp->SetRelativeScale3D(FVector(0.027778,0.027778,0.277778));
	}
	
	// 방패 구현
	ShieldMeshComp=CreateDefaultSubobject<USkeletalMeshComponent>("ShieldMeshComp");
	ShieldMeshComp->SetupAttachment(GetMesh());
	ConstructorHelpers::FObjectFinder<USkeletalMesh> TempShieldMesh(TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
	if (TempShieldMesh.Succeeded())
	{
		ShieldMeshComp->SetSkeletalMesh(TempShieldMesh.Object);
		ShieldMeshComp->SetRelativeLocation(FVector(44.444444,44.444444,55.555556));
		ShieldMeshComp->SetRelativeScale3D(FVector(0.027778,0.277778,0.277778));
	}
}

void ASSRPlayer::BeginPlay()
{
	Super::BeginPlay();
}

void ASSRPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	//이동 관련
	// FVector P0 = GetActorLocation();
	// FVector vt = Direction * MovementSpeed * DeltaTime;
	// SetActorLocation(P0 + vt);
	//
	// Direction = FVector::Zero();
}

void ASSRPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// 플레이어 컨트롤러를 Cast 해서 pc로 초기화
	auto pc = Cast<APlayerController>(Controller);
	if (pc)
	{
		// 서브시스템->로컬플레이어->EnhancedInputLocalPlayerSubsystem
		auto subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer());
		if (subsystem)
		{
			subsystem->AddMappingContext(IMC_SSR, 0);
		}
		
		auto PlayerInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
		
		if (PlayerInput)
		{
			PlayerInput->BindAction(IA_SSRMove,ETriggerEvent::Triggered, this, &ASSRPlayer::Move);
			PlayerInput->BindAction(IA_SSRSwordAttack,ETriggerEvent::Triggered, this, &ASSRPlayer::SwordAttack);
			PlayerInput->BindAction(IA_SSRShieldBlock,ETriggerEvent::Triggered,this, &ASSRPlayer::SheildBlock);
		}
	}
}

void ASSRPlayer::Move(const FInputActionValue& InputActionValue)
{
	FVector2D value = InputActionValue.Get<FVector2D>();
	
	// 입력 받은 값을 방향으로
	FVector TargetDir = FVector(value.X, value.Y, 0.f);

	if (!TargetDir.IsNearlyZero()) // 아무 키도 안눌렀을때 방지
	{
		TargetDir.Normalize();

		// 방향을 회전값으로 변환
		FRotator TargetRot = TargetDir.Rotation();
		SetActorRotation(TargetRot);

		AddMovementInput(GetActorForwardVector(), 1.0f);
	}
	
	// FRotator ControlRot = Controller->GetControlRotation();
	// FRotator YawRot(0.f, ControlRot.Yaw, 0.f);
	//
	// FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	// FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	//
	// AddMovementInput(GetActorForwardVector(), value.X);
	// AddMovementInput(GetActorRightVector(), value.Y);
}

void ASSRPlayer::SwordAttack(const FInputActionValue& InputValue)
{
	UE_LOG(LogTemp, Warning, TEXT("Sword"));
}

void ASSRPlayer::SheildBlock(const FInputActionValue& InputValue)
{
	UE_LOG(LogTemp, Warning, TEXT("Shield"));
}

