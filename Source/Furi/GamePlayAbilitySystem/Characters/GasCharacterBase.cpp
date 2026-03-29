// GasCharacterBase.cpp

#include "GasCharacterBase.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Furi/GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"
#include "Furi/utils/FuriTypes.h"
#include "GameFramework/SpringArmComponent.h"
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
    if (!AbilitySystemComponent) return;

    // 1. 이미 사망 상태라면 리액션을 취하지 않음
    if (AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")))) return;

    // 🌟 [회전 로직] 공격자를 바라보도록 회전
    if (Attacker)
    {
        FVector StartLocation = GetActorLocation();
        FVector TargetLocation = Attacker->GetActorLocation();
        TargetLocation.Z = StartLocation.Z;
        
        FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetLocation);
        SetActorRotation(LookAtRotation);
        
        UE_LOG(LogTemp, Log, TEXT("[HandleDamageResponse] %s rotated to face Attacker: %s"), *GetName(), *Attacker->GetName());
    }

    const EFuriDamageResponse Response = DamageInfo.DamageResponse;
    if (Response == EFuriDamageResponse::None) return;

    // 2. 리액션 몽타주 결정
    UAnimMontage* TargetMontage = nullptr;
    switch (Response)
    {
        case EFuriDamageResponse::HitReaction: TargetMontage = HitReactionMontage; break;
        case EFuriDamageResponse::Stagger:     TargetMontage = StaggerMontage;     break;
        case EFuriDamageResponse::Stun:        TargetMontage = StunMontage;        break;
        case EFuriDamageResponse::KnockBack:   TargetMontage = KnockBackMontage;   break;
    }

    if (!TargetMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HandleDamageResponse] TargetMontage is NULL for Response %d in %s!"), (int32)Response, *GetName());
        return;
    }

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance) return;

    // 3. State.Lock 부여
    const FGameplayTag LockTag = FGameplayTag::RequestGameplayTag(FName("State.Lock"));
    AbilitySystemComponent->AddLooseGameplayTag(LockTag);

    // ---------------------------------------------------------
    // 🎬 [애니메이션 재생]
    // ---------------------------------------------------------
    float ActualLockDuration = TargetMontage->GetPlayLength();

    // 새 몽타주 재생
    PlayAnimMontage(TargetMontage);
    UE_LOG(LogTemp, Log, TEXT("[HandleDamageResponse] Playing Reaction Montage for Response: %d"), (int32)Response);

    // ---------------------------------------------------------
    // ⏱️ [타이머 및 시간 계산 로직]
    // ---------------------------------------------------------
    FTimerManager& TimerManager = GetWorld()->GetTimerManager();
    
    TimerManager.ClearTimer(StunTimerHandle);
    
    if (Response == EFuriDamageResponse::Stun && DamageInfo.StunDuration > 0.0f)
    {
        ActualLockDuration = DamageInfo.StunDuration;

        FTimerDelegate StopMontageDelegate;
        StopMontageDelegate.BindWeakLambda(this, [this, TargetMontage]()
        {
            if (UAnimInstance* SafeAnimInst = GetMesh()->GetAnimInstance())
            {
                if (SafeAnimInst->Montage_IsPlaying(TargetMontage))
                {
                    SafeAnimInst->Montage_Stop(0.2f, TargetMontage);
                }
            }
        });

        TimerManager.SetTimer(StunTimerHandle, StopMontageDelegate, DamageInfo.StunDuration, false);
    }

    TimerManager.ClearTimer(LockTimerHandle);

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
