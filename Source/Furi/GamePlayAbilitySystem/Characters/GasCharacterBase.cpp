// GasCharacterBase.cpp

#include "GasCharacterBase.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Furi/GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"
#include "Furi/utils/FuriTypes.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

AGasCharacterBase::AGasCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// --- GAS 컴포넌트 초기화 ---
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(AscReplicationMode);

	// --- 캐릭터 이동 및 물리 설정 ---
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 800.f;

	// --- 어트리뷰트 세트 초기화 ---
	BasicAttributeSet = CreateDefaultSubobject<UBasicAttributeSet>(TEXT("BasicAttributeSet"));

	// 1. UltSpringArm 생성
	UltSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("UltSpringArm"));
	UltSpringArm->SetupAttachment(RootComponent);
	UltSpringArm->TargetArmLength = 400.0f;
	UltSpringArm->SocketOffset = FVector(0.0f, 0.0f, 50.0f);
	UltSpringArm->bDoCollisionTest = false;
	UltSpringArm->bInheritPitch = false;
	UltSpringArm->bInheritYaw = true;
	UltSpringArm->bInheritRoll = false;

	// 2. UltCamera 생성
	UltCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("UltCamera"));
	UltCamera->SetupAttachment(UltSpringArm, USpringArmComponent::SocketName);
	UltCamera->bUsePawnControlRotation = false;
}

void AGasCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void AGasCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AGasCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AGasCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGasCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

UAbilitySystemComponent* AGasCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGasCharacterBase::HandleDamageResponse(const FFuriDamageInfo& DamageInfo, AActor* Attacker)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// 1. 이미 사망 상태라면 리액션을 취하지 않음
	if (AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead"))))
	{
		return;
	}

	// 🌟 [회전 로직] 공격자를 바라보도록 회전
	if (Attacker)
	{
		FVector StartLocation = GetActorLocation();
		FVector TargetLocation = Attacker->GetActorLocation();
		TargetLocation.Z = StartLocation.Z;

		FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetLocation);
		SetActorRotation(LookAtRotation);

		UE_LOG(LogTemp, Log, TEXT("[HandleDamageResponse] %s rotated to face Attacker: %s"), *GetName(),
		       *Attacker->GetName());
	}

	const EFuriDamageResponse Response = DamageInfo.DamageResponse;
	if (Response == EFuriDamageResponse::None)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("DamageAmount : %f"), DamageInfo.Amount);
	if (DamageInfo.Amount < 0)
	{
		float HitStopDuration = 0.05f; // 기본 히트스탑 시간

		switch (DamageInfo.DamageResponse)
		{
		case EFuriDamageResponse::KnockBack:
			HitStopDuration = 0.1f; // 강한 공격은 더 길게 멈춤
			break;
		case EFuriDamageResponse::Stun:
			HitStopDuration = 0.1f;
			break;
		case EFuriDamageResponse::HitReaction:
			HitStopDuration = 0.05f; // 일반 공격은 짧게 멈춤
			break;
		}

		// 히트스탑 실행! (0.01배속으로 멈춤)
		ExecuteHitStop(HitStopDuration, 0.2f);
	}

	// 2. 리액션 몽타주 결정
	UAnimMontage* TargetMontage = nullptr;
	switch (Response)
	{
	case EFuriDamageResponse::HitReaction: TargetMontage = HitReactionMontage;
		break;
	case EFuriDamageResponse::Stagger: TargetMontage = StaggerMontage;
		break;
	case EFuriDamageResponse::Stun: TargetMontage = StunMontage;
		break;
	case EFuriDamageResponse::KnockBack: TargetMontage = KnockBackMontage;
		break;
	}

	if (!TargetMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleDamageResponse] TargetMontage is NULL for Response %d in %s!"),
		       (int32)Response, *GetName());
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	// ---------------------------------------------------------
	// State.Lock 부여 및 스택 중첩 방지
	// ---------------------------------------------------------
	const FGameplayTag LockTag = FGameplayTag::RequestGameplayTag(FName("State.Lock"));
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();

	// 만약 이전 피격으로 인해 이미 타이머가 돌고 있다면 (연속 피격)
	if (TimerManager.IsTimerActive(LockTimerHandle))
	{
		// 타이머를 취소하고, 취소된 타이머가 지웠어야 할 태그 스택을 미리 하나 상쇄시킵니다.
		TimerManager.ClearTimer(LockTimerHandle);
		AbilitySystemComponent->RemoveLooseGameplayTag(LockTag);
	}

	// 이제 안전하게 새 태그 스택을 추가합니다.
	AbilitySystemComponent->AddLooseGameplayTag(LockTag);

	// ---------------------------------------------------------
	// 🎬 [애니메이션 재생]
	// ---------------------------------------------------------
	float ActualLockDuration = TargetMontage->GetPlayLength();

	// 기존: PlayAnimMontage(TargetMontage);
	// 변경: 서버가 모든 클라이언트에게 재생하라고 방송함
	Multicast_PlayReactionMontage(TargetMontage);

	// ---------------------------------------------------------
	// ⏱️ [타이머 및 시간 계산 로직]
	// ---------------------------------------------------------
	TimerManager.ClearTimer(StunTimerHandle);

	if (Response == EFuriDamageResponse::Stun && DamageInfo.StunDuration > 0.0f)
	{
		ActualLockDuration = DamageInfo.StunDuration;

		FTimerDelegate StopMontageDelegate;
		StopMontageDelegate.BindWeakLambda(this, [this, TargetMontage]()
		{
			// 기존: 내부에서 직접 Montage_Stop 호출
			// 변경: 서버가 모든 클라이언트에게 정지하라고 방송함
			Multicast_StopReactionMontage(TargetMontage);
		});

		TimerManager.SetTimer(StunTimerHandle, StopMontageDelegate, DamageInfo.StunDuration, false);
	}

	// [Lock 해제 로직] (위에서 ClearTimer를 했으므로 바로 SetTimer만 하면 됩니다)
	FTimerDelegate RemoveLockDelegate;
	RemoveLockDelegate.BindWeakLambda(this, [this, LockTag]()
	{
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->RemoveLooseGameplayTag(LockTag);
		}
	});

	float SafeLockDuration = FMath::Max(ActualLockDuration, 0.1f);
	TimerManager.SetTimer(LockTimerHandle, RemoveLockDelegate, SafeLockDuration, false);
}

// 🌟 모든 클라이언트 화면에서 몽타주 재생
void AGasCharacterBase::Multicast_PlayReactionMontage_Implementation(UAnimMontage* MontageToPlay)
{
	if (MontageToPlay)
	{
		PlayAnimMontage(MontageToPlay);
		UE_LOG(LogTemp, Log, TEXT("[Client/Server] Playing Montage: %s"), *MontageToPlay->GetName());
	}
}

// 🌟 모든 클라이언트 화면에서 몽타주 정지 (스턴 종료 시 등)
void AGasCharacterBase::Multicast_StopReactionMontage_Implementation(UAnimMontage* MontageToStop)
{
	if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
	{
		if (AnimInst->Montage_IsPlaying(MontageToStop))
		{
			AnimInst->Montage_Stop(0.2f, MontageToStop);
		}
	}
}

void AGasCharacterBase::ExecuteHitStop(float Duration, float TimeScale)
{
	if (Duration <= 0.f || !GetWorld())
	{
		return;
	}

	// 1. [핵심] 현재 히트스탑 중이 아닐 때만, 원래 시간 배속(궁극기 0.3 등)을 기억해 둡니다.
	// 이렇게 해야 연타를 맞아도 0.01배속을 원래 속도로 착각하고 저장하는 버그가 안 생깁니다.
	if (!bIsHitStopping)
	{
		PreHitStopTimeDilation = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
		bIsHitStopping = true;
	}

	// 2. 이미 예약된 히트스탑 종료 티커가 있다면 취소 (연속 타격 시 히트스탑 시간 연장)
	if (HitStopTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(HitStopTickerHandle);
	}

	// 3. 세상을 히트스탑 배속(0.01)으로 멈춥니다.
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), TimeScale);

	// 4. 현실 시간 기준으로 티커 예약
	HitStopTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this](float DeltaTime)
		{
			StopHitStop();
			return false; // false 리턴 시 티커 1회 실행 후 파괴
		}),
		Duration
	);
}

void AGasCharacterBase::StopHitStop()
{
	if (GetWorld() && bIsHitStopping)
	{
		// 5. [핵심] 무조건 1.0으로 돌리는 게 아니라, 기억해둔 이전 배속(궁극기 중이었다면 0.3)으로 안전하게 복구합니다.
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), PreHitStopTimeDilation);

		bIsHitStopping = false;
		HitStopTickerHandle.Reset();
	}
}
