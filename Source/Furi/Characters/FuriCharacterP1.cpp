// Fill out your copyright notice in the Description page of Project Settings.

#include "FuriCharacterP1.h"
#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include "Furi/Weapons/WeaponDataAsset.h"
#include "Furi/Weapons/WeaponManagerComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"


// Sets default values
AFuriCharacterP1::AFuriCharacterP1()
{
	// 틱 설정
	PrimaryActorTick.bCanEverTick = true;
	//P1 카메라 기본 세팅
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	
	//P1 메시 기본 세팅
	GetMesh()->SetRelativeScale3D(FVector(0.21f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	ConstructorHelpers::FObjectFinder<USkeletalMesh> TempMesh(TEXT("/Script/Engine.SkeletalMesh'/Game/P1/Characters/Meshs/ue4_inosuke.ue4_inosuke'"));
	if (TempMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(TempMesh.Object);
	}
	
	//Input 관련 세팅
	ConstructorHelpers::FObjectFinder<UInputMappingContext> TempIMC(TEXT("/Script/EnhancedInput.InputMappingContext'/Game/P1/input/IMC_P1Default.IMC_P1Default'"));
	if (TempIMC.Succeeded())
	{
		IMC_P1 = TempIMC.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempMoveAction(TEXT("/Script/EnhancedInput.InputAction'/Game/P1/input/Action/IA_P1Move.IA_P1Move'"));
	if (TempMoveAction.Succeeded())	{
		MoveAction = TempMoveAction.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempLookAction(TEXT("/Script/EnhancedInput.InputAction'/Game/P1/input/Action/IA_P1Look.IA_P1Look'"));
	if (TempLookAction.Succeeded())
	{
		LookAction = TempLookAction.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempMouseLookAction(TEXT("/Script/EnhancedInput.InputAction'/Game/P1/input/Action/IA_P1MouseLook.IA_P1MouseLook'"));
	if (TempMouseLookAction.Succeeded())
	{
		MouseLookAction = TempMouseLookAction.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempDashAction(TEXT("/Script/EnhancedInput.InputAction'/Game/P1/input/Action/IA_P1Dash.IA_P1Dash'"));
	if (TempDashAction.Succeeded())
	{
		DashAction = TempDashAction.Object;
	}
	
	//기본속도 800으로 설정
	GetCharacterMovement()->MaxWalkSpeed = 800.f;
	
	//무기 매니저 컴포넌트 생성 및 등록
	WeaponManager = CreateDefaultSubobject<UWeaponManagerComponent>(TEXT("WeaponManager"));
}

// Called when the game starts or when spawned
void AFuriCharacterP1::BeginPlay()
{
	Super::BeginPlay();
	
	// 게임 시작 시 기본 무기 장착
	if (WeaponManager && DefaultWeaponData)
	{
		WeaponManager->EquipWeapon(DefaultWeaponData->WeaponConfig);
	}
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

