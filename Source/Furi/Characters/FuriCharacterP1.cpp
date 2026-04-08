// Fill out your copyright notice in the Description page of Project Settings.

#include "FuriCharacterP1.h"
#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Furi/Weapons/WeaponDataAsset.h"
#include "Furi/Weapons/WeaponManagerComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AFuriCharacterP1::AFuriCharacterP1()
{
	// 틱 설정
	PrimaryActorTick.bCanEverTick = true;

	//P1 메시 기본 세팅
	GetMesh()->SetRelativeScale3D(FVector(0.21f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	ConstructorHelpers::FObjectFinder<USkeletalMesh> TempMesh(
		TEXT("/Script/Engine.SkeletalMesh'/Game/P1/Characters/Meshs/ue4_inosuke.ue4_inosuke'"));
	if (TempMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(TempMesh.Object);
	}

	//Input 관련 세팅
	ConstructorHelpers::FObjectFinder<UInputMappingContext> TempIMC(
		TEXT("/Script/EnhancedInput.InputMappingContext'/Game/P1/input/IMC_P1Default.IMC_P1Default'"));
	if (TempIMC.Succeeded())
	{
		IMC_P1 = TempIMC.Object;
	}
	ConstructorHelpers::FObjectFinder<UInputAction> TempMoveAction(
		TEXT("/Script/EnhancedInput.InputAction'/Game/P1/input/Action/IA_P1Move.IA_P1Move'"));
	if (TempMoveAction.Succeeded())
	{
		MoveAction = TempMoveAction.Object;
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
			subsysem->AddMappingContext(IMC_P1, 0);
		}
		auto playerInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
		if (playerInputComponent)
		{
			// Moving
			playerInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFuriCharacterP1::Move);

			for (const FFuriInputActionConfig& Config : AbilityInputConfigs)
			{
				if (Config.InputAction && Config.InputTag.IsValid())
				{
					// 키를 눌렀을 때 AbilityInputTagPressed 호출
					playerInputComponent->BindAction(Config.InputAction, ETriggerEvent::Started, this,
					                                 &AFuriCharacterP1::AbilityInputTagPressed, Config.InputTag);
				}
			}
		}
	}
}

void AFuriCharacterP1::Move(const FInputActionValue& Value)
{
	// 캐릭터에게 AbilityLock 또는 Dead 태그가 있다면 이동 입력을 즉시 리턴(무시)합니다.
	if (GetAbilitySystemComponent()->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Lock"))) ||
		GetAbilitySystemComponent()->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead"))))
	{
		return;
	}

	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// 🌟 [수정 전] 보통 Controller->GetControlRotation()을 사용하셨을 겁니다.
		// 🌟 [수정 후] 카메라의 회전값을 기준으로 가져옵니다!
        
		FRotator Rotation = FRotator::ZeroRotator;

		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			// 현재 화면을 비추고 있는 카메라(MainCameraActor)의 정보를 가져옵니다.
			if (AActor* CameraActor = PC->GetViewTarget())
			{
				Rotation = CameraActor->GetActorRotation();
			}
		}

		// 카메라의 Z축(Yaw) 회전값만 남겨서 바닥과 평행한 방향을 구합니다.
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// 카메라가 바라보는 '앞'과 '오른쪽' 방향을 계산합니다.
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// 입력받은 값에 따라 이동을 적용합니다.
		AddMovementInput(ForwardDirection, MovementVector.Y); // W, S 입력
		AddMovementInput(RightDirection, MovementVector.X);   // A, D 입력
	}
}

void AFuriCharacterP1::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 1. GAS Actor Info 초기화
	InitAbilityActorInfo();

	// 2. 어빌리티 부여
	if (AbilitySystemComponent)
	{
		for (TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
		{
			if (AbilityClass)
			{
				// 어빌리티 스펙을 생성하여 부여
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass));
			}
		}
		if (StaminaRegenEffectClass)
		{
			// 이펙트를 적용하기 위한 문맥(Context) 생성 (누가 누구에게 거는가)
			FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
			ContextHandle.AddInstigator(this, this);

			// 이펙트 스펙(Spec) 생성
			FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
				StaminaRegenEffectClass, 1.0f, ContextHandle);

			if (SpecHandle.IsValid())
			{
				// 생성된 이펙트를 내 몸(Self)에 영구적으로 적용합니다.
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}

void AFuriCharacterP1::InitAbilityActorInfo()
{
	// ASC에 이 캐릭터가 Owner이자 Avatar임을 알립니다.
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AFuriCharacterP1::AbilityInputTagPressed(FGameplayTag InputTag)
{
	//InputTag 로그 추가
	UE_LOG(LogTemp, Warning, TEXT("AbilityInputTagPressed called with Tag: %s"), *InputTag.ToString());
	if (!AbilitySystemComponent || !InputTag.IsValid())
	{
		return;
	}

	bool bIsAlreadyActive = false; // 켜져 있는지 체크

	for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(InputTag))
		{
			if (Spec.IsActive())
			{
				// 이미 켜져 있으면 콤보 신호만 줌
				AbilitySystemComponent->AbilitySpecInputPressed(Spec);
				UE_LOG(LogTemp, Warning, TEXT("Sent InputPressed to Active Ability!"));
				bIsAlreadyActive = true;
			}
		}
	}

	// 켜져 있지 않을 때만 새로 실행함
	if (!bIsAlreadyActive)
	{
		FGameplayTagContainer AbilityTagsToActivate;
		AbilityTagsToActivate.AddTag(InputTag);
		AbilitySystemComponent->TryActivateAbilitiesByTag(AbilityTagsToActivate);
	}
}
