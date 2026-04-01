// Fill out your copyright notice in the Description page of Project Settings.


#include "SSRPlayer.h"
#include "SSRSword.h"

#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Furi/Weapons/WeaponDataAsset.h"
#include "Furi/Weapons/WeaponManagerComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ASSRPlayer::ASSRPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true; // 원래는 false
	
	bReplicates = true;
	ACharacter::SetReplicateMovement(true);

	// ⭐ 핵심: 컨트롤러의 회전(카메라 각도 등)을 서버에 복제합니다.
	bNetUseOwnerRelevancy = true;
	
	WeaponManager = CreateDefaultSubobject<UWeaponManagerComponent>(TEXT("WeaponManger"));
	
}

void ASSRPlayer::BeginPlay()
{
	Super::BeginPlay();
	
	// if (SwordClass)
	// {
	// 	CurrentWeapon = GetWorld()->SpawnActor<ASSRSword>(SwordClass);
	// 	
	// 	if (CurrentWeapon)
	// 	{
	// 		FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
	// 		CurrentWeapon->AttachToComponent(
	// 			GetMesh(),
	// 			AttachRules,TEXT("WeaponSocket")
	// 			);
	// 	}
	// }
	
	if (WeaponManager && DefaultWeaponData)
	{
		WeaponManager->EquipWeapon(DefaultWeaponData->WeaponConfig);
	}
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
			// PlayerInput->BindAction(IA_SSRSwordAttack,ETriggerEvent::Started, this, &ASSRPlayer::SwordAttack);
			// PlayerInput->BindAction(IA_SSRShieldBlock,ETriggerEvent::Started,this, &ASSRPlayer::SheildBlock);
			// PlayerInput->BindAction(IA_SSRDash,ETriggerEvent::Started,this, &ASSRPlayer::Dash);
			// PlayerInput->BindAction(IA_SSRSunFire,ETriggerEvent::Started,this, &ASSRPlayer::SunFire);
			for (const FFuriInputActionConfig& Config : AbilityInputConfigs)
			{
				if (Config.InputAction && Config.InputTag.IsValid())
				{
					// 키를 눌렀을 때 AbilityInputTagPressed 호출
					PlayerInput->BindAction(Config.InputAction, ETriggerEvent::Started, this, &ASSRPlayer::AbilityInputTagPressed, Config.InputTag);
					// 2. 키를 떼었을 때 (Completed) -> 이게 있어야 WaitInputRelease가 작동합니다!
					PlayerInput->BindAction(Config.InputAction, ETriggerEvent::Completed, this, &ASSRPlayer::AbilityInputTagReleased, Config.InputTag);
                
					// // 참고: 트리거 설정에 따라 Canceled도 추가해야 할 수 있습니다.
					// PlayerInput->BindAction(Config.InputAction, ETriggerEvent::Canceled, this, &ASSRPlayer::AbilityInputTagReleased, Config.InputTag);
				}
			}
		}
	}
}

void ASSRPlayer::Move(const FInputActionValue& InputActionValue)
{
	// 캐릭터에게 AbilityLock 태그가 있다면 이동 입력을 즉시 리턴(무시)합니다.
	if (GetAbilitySystemComponent()->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Lock"))))
	{
		return;
	}
	
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

void ASSRPlayer::Dash(const FInputActionValue& InputValue)
{
	UE_LOG(LogTemp, Warning, TEXT("Shield"));
}

void ASSRPlayer::SunFire(const FInputActionValue& InputValue)
{
	UE_LOG(LogTemp, Warning, TEXT("SunFire"));
}

void ASSRPlayer::SwordSkill2(const FInputActionValue& InputValue)
{
	UE_LOG(LogTemp, Warning, TEXT("Shield"));
}

void ASSRPlayer::SwordSkill3(const FInputActionValue& InputValue)
{
}

void ASSRPlayer::PossessedBy(AController* NewController)
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
			// 이펙트를 적용하기 위한 문맥(Context) 생성 -> 누가 누구에게 거는가
			FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
			ContextHandle.AddInstigator(this, this);
			
			// 이펙트 스펙(Spec) 생성
			FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(StaminaRegenEffectClass, 1.0f, ContextHandle);
			
			if (SpecHandle.IsValid())
			{
				// 생성된 이펙트를 내 몸(Self)에 영구적으로 적용합니다.
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}

void ASSRPlayer::InitAbilityActorInfo()
{
	//ASC에 이 캐릭터가 Owner이자 Avater임을 밝힙니다
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this,this);
	}
}

void ASSRPlayer::AbilityInputTagPressed(FGameplayTag InputTag)
{
	UE_LOG(LogTemp, Warning, TEXT("AbilityInputTagPressed %s"), *InputTag.ToString());
    
	if (!AbilitySystemComponent || !InputTag.IsValid()) return;

	bool bIsAlreadyActive = false;
    
	// 1. 현재 활성화된 어빌리티들 중 해당 태그를 가진 녀석이 있는지 확인
	for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(InputTag))
		{
			if (Spec.IsActive())
			{
				//실행 중이라면 입력 신호(InputPressed)만 보내고 끝냅니다!
				AbilitySystemComponent->AbilitySpecInputPressed(Spec);
				
				bIsAlreadyActive = true;
				UE_LOG(LogTemp, Log, TEXT("Existing Ability found. Sent InputPressed."));
				break; // 찾았으니 루프 탈출
			}
		}
	}
    
	// 2. 실행 중인 게 없을 때만 새로 실행
	if (!bIsAlreadyActive)
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(InputTag));
		UE_LOG(LogTemp, Log, TEXT("No Active Ability. Trying to Activate New."));
	}
}

void ASSRPlayer::AbilityInputTagReleased(FGameplayTag InputTag)
{
	// 로그를 찍어 실제 버튼을 뗄 때 호출되는지 확인하세요!
	UE_LOG(LogTemp, Warning, TEXT("AbilityInputTagReleased %s"), *InputTag.ToString());

	if (!AbilitySystemComponent || !InputTag.IsValid()) return;

	// 현재 활성화된 어빌리티들 중 해당 태그를 가진 녀석에게 "입력 해제" 신호 전달
	for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(InputTag))
		{
			if (Spec.IsActive())
			{
				// [핵심] 이 함수가 호출되어야 WaitInputRelease 태스크가 완료됩니다!
				AbilitySystemComponent->AbilitySpecInputReleased(Spec);
				UE_LOG(LogTemp, Log, TEXT("Sent InputReleased to Active Ability."));
			}
		}
	}
}
