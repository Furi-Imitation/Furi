// GasCharacterBase.cpp

#include "GasCharacterBase.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Furi/FuriGameMode.h"
#include "Furi/FuriPlayerController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
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

// 서버(또는 로컬)에서 사망 판정이 났을 때 실행되는 메인 함수
void AGasCharacterBase::Die()
{
	UE_LOG(LogTemp, Error, TEXT("==== [DIE] %s has entered Die() ===="), *GetName());

	if (!AbilitySystemComponent)
	{
		return;
	}

	FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(FName("State.Dead"));

	// 1. 이미 죽어있다면 무시 (중복 사망 방지)
	if (AbilitySystemComponent->HasMatchingGameplayTag(DeadTag))
	{
		return;
	}

	// 2. 사망 태그 부여 (HandleDamageResponse의 최상단에서 데미지를 거부하게 됨)
	AbilitySystemComponent->AddLooseGameplayTag(DeadTag);

	// 3. 현재 시전 중이거나 유지 중인 모든 스킬(어빌리티) 강제 종료
	AbilitySystemComponent->CancelAllAbilities();

	// 4. 사망 연출 및 물리 처리를 모든 사람의 화면(클라이언트)에 방송
	Multicast_Die();

	UE_LOG(LogTemp, Warning, TEXT("[%s] 사망했습니다!"), *GetName());

	// 0408
	
	if (HasAuthority()) // 서버에서만 실행
	{
		AFuriGameMode* GM = Cast<AFuriGameMode>(GetWorld()->GetAuthGameMode());
		if (GM)
		{
			// 1대1 상황이므로:
			// Loser(패자): 죽은 본인 (this)
			// Winner(승자): 내가 아닌 다른 플레이어 캐릭터
            
			AGasCharacterBase* Winner = nullptr;
            
			// 월드에서 내가 아닌 다른 캐릭터 하나를 찾습니다.
			TArray<AActor*> FoundCharacters;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGasCharacterBase::StaticClass(), FoundCharacters);
			for (AActor* Actor : FoundCharacters)
			{
				if (Actor != this)
				{
					Winner = Cast<AGasCharacterBase>(Actor);
					break;
				}
			}

			// 이제 GameMode에게 연출 시퀀스를 시작하라고 명령합니다.
			if (Winner)
			{
				GM->ProcessMatchEnd(Winner, this);
			}
			// else
			// {
			// 	// 승자를 못 찾은 경우를 대비한 안전장치 (기존처럼 UI라도 띄움)
			// 	if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(GetController()))
			// 	{
			// 		PC->Client_ShowGameEndUI(false);
			// 	}
			// }
		}
	}
	
	// 0408
	
	// // ==========================================
	// // 4. 게임 종료 UI 호출 로직 추가
	// // ==========================================
	//
	// if (HasAuthority()) // 서버에서만 판단합니다.
	// {
	// 	AController* MyController = GetController();
	//
	// 	// 1. 죽은 게 플레이어라면 -> 그 플레이어에게 "패배" 전달
	// 	if (AFuriPlayerController* PC = Cast<AFuriPlayerController>(MyController))
	// 	{
	// 		PC->Client_ShowGameEndUI(false);
	// 	}
	//
	// 	// 2. 적이 죽었다면 -> 월드의 모든 플레이어에게 "승리" 전달
	// 	// (1대1 게임이라면 상대방만 찾아서 보내면 됩니다)
	// 	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	// 	{
	// 		if (AFuriPlayerController* OtherPC = Cast<AFuriPlayerController>(It->Get()))
	// 		{
	// 			// 죽은 본인이 아닌 사람들에게만 승리라고 알려줌
	// 			if (OtherPC != MyController)
	// 			{
	// 				OtherPC->Client_ShowGameEndUI(true);
	// 			}
	// 		}
	// 	}
	// }
}

void AGasCharacterBase::PlayDeathMontage()
{
	if (DeathMontage)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			bDeathFinishNotified = false;
			BindFinishingMontageNotifies();
			AnimInstance->Montage_Play(DeathMontage);
			bIsWinningSequence = false;

			// 🌟 몽타주가 끝날 때 실행될 함수 연결
			FOnMontageEnded EndDelegate;
			// EndDelegate.BindUObject(this, &AGasCharacterBase::OnFinishMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, DeathMontage);
		}
	}
}

void AGasCharacterBase::PlayVictoryMontage()
{
	if (VictoryMontage)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			bVictoryFinishNotified = false;
			BindFinishingMontageNotifies();
			AnimInstance->Montage_Play(VictoryMontage);
			bIsWinningSequence = true;

			FOnMontageEnded EndDelegate;
			// EndDelegate.BindUObject(this, &AGasCharacterBase::OnFinishMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, VictoryMontage);
		}
	}
}

void AGasCharacterBase::BindFinishingMontageNotifies()
{
	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &AGasCharacterBase::HandleMontageNotifyBegin);
		AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &AGasCharacterBase::HandleMontageNotifyBegin);
	}
}

void AGasCharacterBase::HandleMontageNotifyBegin(FName NotifyName,
                                                 const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (NotifyName == TEXT("DieFinish") && !bDeathFinishNotified)
	{
		bDeathFinishNotified = true;
		AnimNotify_DieFinish();
		return;
	}

	if (NotifyName == TEXT("VictoryFinish") && !bVictoryFinishNotified)
	{
		bVictoryFinishNotified = true;
		AnimNotify_VictoryFinish();
	}
}

// void AGasCharacterBase::OnFinishMontageEnded(UAnimMontage* Montage, bool bInterrupted)
// {
// 	if (!bIsWinningSequence)
// 	{
// 		SetActorHiddenInGame(true);
// 		SetActorEnableCollision(false);
// 	}
// 	
// 	if (HasAuthority())
// 	{
// 		AFuriGameMode* GM = Cast<AFuriGameMode>(GetWorld()->GetAuthGameMode());
// 		if (GM)
// 		{
// 			if (bIsWinningSequence)
// 			{
// 				GM->OnVictoryAnimationFinished();
// 			}
// 			else
// 			{
// 				GM->OnDeathAnimationFinished();
// 			}
// 		}
// 	}
// }

void AGasCharacterBase::AnimNotify_DieFinish()
{
	UE_LOG(LogTemp, Warning, TEXT("DieFinish Notify Received on %s"), HasAuthority() ? TEXT("Server") : TEXT("Client"));

	// 서버에게 보고합니다.
	Server_NotifyDieFinish();
}

void AGasCharacterBase::AnimNotify_VictoryFinish()
{
	Server_NotifyVictoryFinish();
}

void AGasCharacterBase::Server_NotifyVictoryFinish_Implementation()
{
	if (AFuriGameMode* GM = Cast<AFuriGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->OnVictoryAnimationFinished();
	}
}

void AGasCharacterBase::Server_NotifyDieFinish_Implementation()
{
	AFuriGameMode* GM = Cast<AFuriGameMode>(GetWorld()->GetAuthGameMode());
	if (GM)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server: Calling GM->OnDeathAnimationFinished()"));
		GM->OnDeathAnimationFinished();
	}
}


// 모든 클라이언트의 화면에서 동일하게 처리되는 시각적/물리적 사망 처리
void AGasCharacterBase::Multicast_Die_Implementation()
{
	// 1. 캡슐 콜리전 비활성화 (시체가 길을 막거나 칼을 막아버리는 현상 방지)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 2. 이동 완전 차단
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();

	// // 3. 사망 몽타주 재생 (없으면 래그돌로 전환)
	// if (DeathMontage)
	// {
	// 	PlayAnimMontage(DeathMontage);
	// }
	// else
	// {
	// 	//사망 몽타주가 세팅되지 않았을 경우, 자연스럽게 물리 엔진에 맡겨 쓰러지게(Ragdoll) 만듭니다.
	// 	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	// 	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	// 	GetMesh()->SetSimulatePhysics(true);
	// }
	if (!DeathMontage)
	{
		//사망 몽타주가 세팅되지 않았을 경우, 자연스럽게 물리 엔진에 맡겨 쓰러지게(Ragdoll) 만듭니다.
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetSimulatePhysics(true);
	}
}
