// Fill out your copyright notice in the Description page of Project Settings.

#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "FuriCharacterP1.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"


// Sets default values
AFuriCharacterP1::AFuriCharacterP1()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

// Called when the game starts or when spawned
void AFuriCharacterP1::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFuriCharacterP1::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AFuriCharacterP1::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	APlayerController* pc = Cast<APlayerController>(Controller);
	if (pc)
	{
		auto subsysem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer());
		
		if (subsysem)
		{
			subsysem->AddMappingContext(IMC_P1,0);
		}
		auto playerInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
		if (playerInputComponent)
		{
			// Moving
			playerInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFuriCharacterP1::Move);
			playerInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AFuriCharacterP1::Look);

			// Looking
			playerInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFuriCharacterP1::Look);
		}
	}
}

void AFuriCharacterP1::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AFuriCharacterP1::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AFuriCharacterP1::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AFuriCharacterP1::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AFuriCharacterP1::Dash(const FInputActionValue& inputValue)
{
}

