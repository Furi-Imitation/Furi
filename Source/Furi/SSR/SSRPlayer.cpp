// Fill out your copyright notice in the Description page of Project Settings.


#include "SSRPlayer.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "SSRSword.h"

#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Components/BoxComponent.h"
#include "Furi/Weapons/WeaponDataAsset.h"
#include "Furi/Weapons/WeaponManagerComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

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

	// 필살기 충돌체 생성 및 설정
	UltimateHitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("UltimateHitBox"));
	UltimateHitBox->SetupAttachment(GetMesh(), FName("WeaponSocket")); // 무기 위치에 부착 권장
	UltimateHitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
	UltimateHitBox->OnComponentBeginOverlap.AddDynamic(this, &ASSRPlayer::OnUltimateOverlap);
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
			PlayerInput->BindAction(IA_SSRMove, ETriggerEvent::Triggered, this, &ASSRPlayer::Move);
			for (const FFuriInputActionConfig& Config : AbilityInputConfigs)
			{
				if (Config.InputAction && Config.InputTag.IsValid())
				{
					// 키를 눌렀을 때 AbilityInputTagPressed 호출
					PlayerInput->BindAction(Config.InputAction, ETriggerEvent::Started, this,
					                        &ASSRPlayer::AbilityInputTagPressed, Config.InputTag);
					// 2. 키를 떼었을 때 (Completed) -> 이게 있어야 WaitInputRelease가 작동합니다!
					PlayerInput->BindAction(Config.InputAction, ETriggerEvent::Completed, this,
					                        &ASSRPlayer::AbilityInputTagReleased, Config.InputTag);

					// // 참고: 트리거 설정에 따라 Canceled도 추가해야 할 수 있습니다.
					// PlayerInput->BindAction(Config.InputAction, ETriggerEvent::Canceled, this, &ASSRPlayer::AbilityInputTagReleased, Config.InputTag);
				}
			}
		}
	}
}

void ASSRPlayer::Move(const FInputActionValue& InputActionValue)
{
	if (GetAbilitySystemComponent()->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Lock"))) ||
		GetAbilitySystemComponent()->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead"))))
	{
		return;
	}

	FVector2D MovementVector = InputActionValue.Get<FVector2D>();

	if (Controller != nullptr)
	{
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
		AddMovementInput(RightDirection, MovementVector.X); // A, D 입력
	}
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

void ASSRPlayer::InitAbilityActorInfo()
{
	//ASC에 이 캐릭터가 Owner이자 Avater임을 밝힙니다
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void ASSRPlayer::AbilityInputTagPressed(FGameplayTag InputTag)
{
	UE_LOG(LogTemp, Warning, TEXT("AbilityInputTagPressed %s"), *InputTag.ToString());

	if (!AbilitySystemComponent || !InputTag.IsValid())
	{
		return;
	}

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

	if (!AbilitySystemComponent || !InputTag.IsValid())
	{
		return;
	}

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

void ASSRPlayer::SetUltimateCollisionEnabled(bool bEnable)
{
	// 🌟 안전장치 추가: 박스가 유효할 때만 실행
	if (UltimateHitBox)
	{
		UltimateHitBox->SetCollisionEnabled(bEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SSRPlayer: UltimateHitBox is NULL! Check if it's created in Constructor."));
	}
}

void ASSRPlayer::SetCameraZoom(bool bZoomIn)
{
}

void ASSRPlayer::OnUltimateOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                   const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this)
	{
		// 1. 중복 방지를 위해 충돌체 즉시 끄기
		SetUltimateCollisionEnabled(false);

		// 2. 어빌리티에게 "적중 성공" 이벤트 전송 (Payload에 타겟 정보 포함)
		FGameplayEventData Payload;
		Payload.Target = OtherActor;
		Payload.Instigator = this;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			this, FGameplayTag::RequestGameplayTag(FName("Event.SSRFinal.HitSuccess")), Payload);
	}
}
