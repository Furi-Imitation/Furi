// GasCharacterBase.cpp

#include "GasCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Furi/GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"
#include "Furi/utils/FuriTypes.h"

AGasCharacterBase::AGasCharacterBase()
{
    PrimaryActorTick.bCanEverTick = true;

    // --- GAS 컴포넌트 초기화 ---
    // 캐릭터의 모든 능력(Ability)을 처리하는 핵심 컴포넌트를 생성합니다.
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    
    // 네트워크 복제 활성화: 서버의 스킬 실행 상태를 클라이언트에 동기화합니다.
    AbilitySystemComponent->SetIsReplicated(true);
    
    // 복제 모드 설정: 속성(Attribute) 변화를 네트워크를 통해 얼마나 상세히 보낼지 결정합니다.
    // (보통 혼자 조종하는 캐릭터는 Mixed, AI는 Minimal을 사용합니다.)
    AbilitySystemComponent->SetReplicationMode(AscReplicationMode);
    
    // --- 캐릭터 이동 및 물리 설정 ---
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
       
    // 카메라 회전이 캐릭터 몸체 회전에 직접 영향을 주지 않도록 설정 (TPS 게임 기본)
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // 캐릭터가 이동하는 방향으로 몸을 자동으로 회전시킵니다.
    GetCharacterMovement()->bOrientRotationToMovement = true; 
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

    // 점프력 및 공중 제어력, 이동 속도 등 물리 수치 설정
    GetCharacterMovement()->JumpZVelocity = 500.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 800.f;
    
    // --- 어트리뷰트 세트 초기화 ---
    // HP, MP 등 수치 데이터를 저장하는 클래스를 생성하여 ASC와 연결합니다.
    BasicAttributeSet = CreateDefaultSubobject<UBasicAttributeSet>(TEXT("BasicAttributeSet"));
}

void AGasCharacterBase::BeginPlay()
{
    Super::BeginPlay();
}

// [서버 전용] 컨트롤러가 이 캐릭터를 점유했을 때 호출됩니다.
void AGasCharacterBase::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    
    if (AbilitySystemComponent)
    {
       // 서버에서 '능력 정보(Actor Info)'를 초기화합니다.
       // Owner(소유자)와 Avatar(실행자)를 모두 이 캐릭터(this)로 설정합니다.
       AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }
}

// [클라이언트 전용] 플레이어 상태(PlayerState)가 복제되었을 때 호출됩니다.
void AGasCharacterBase::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    
    if (AbilitySystemComponent)
    {
       // 클라이언트에서도 '능력 정보'를 초기화해야 이펙트 재생 및 UI 연동이 가능합니다.
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

// 인터페이스 구현: 외부 클래스들이 이 캐릭터의 ASC에 접근할 때 사용합니다.
UAbilitySystemComponent* AGasCharacterBase::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AGasCharacterBase::TakeFuriDamage(const FFuriDamageInfo& DamageInfo, const FGameplayEffectSpecHandle& DamageSpec, AActor* InstigatorActor)
{
    if (!AbilitySystemComponent) return;

    // 🌟 이미 죽었다면 대미지나 리액션을 무시합니다. (BasicAttributeSet가 유효한지 확인)
    if (BasicAttributeSet && BasicAttributeSet->GetHealth() <= 0.0f)
    {
        return;
    }

    // 🛡️ 1. 액션 판정 로직 (무적, 가드, 패링 검사)
    // 무적 판정
    bool bIsInvincible = AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Invincible")));
    if (bIsInvincible && !DamageInfo.bShouldDamageInvincible)
    {
        UE_LOG(LogTemp, Log, TEXT("Damage Evaded or Invincible!"));
        return; // 맞지 않음 (회피 성공)
    }

    // 가드 판정
    bool bIsBlocking = AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Blocking")));
    if (bIsBlocking && DamageInfo.bCanBeBlocked)
    {
        UE_LOG(LogTemp, Log, TEXT("Attack Blocked!"));
        // TODO: 가드 성공 애니메이션(GuardReaction) 재생 및 가드 전용 스태미나 차감 로직 추가 가능
        return; // 가드 성공 시 일반 피격 로직 및 대미지 무시
    }


    // =========================================================
    // 💥 2. 피격 확정: 애니메이션 및 리액션(넉백) 처리
    // =========================================================
    
    switch (DamageInfo.DamageResponse)
    {
        case EFuriDamageResponse::HitReaction:
            // 1. 일반 피격
            if (HitReactionMontage)
            {
                PlayAnimMontage(HitReactionMontage);
            }
            break;

        case EFuriDamageResponse::Stagger:
            // 2. 경직
            if (StaggerMontage)
            {
                PlayAnimMontage(StaggerMontage);
            }
            break;

        case EFuriDamageResponse::Stun:
            // 3. 기절
            UE_LOG(LogTemp, Warning, TEXT("Character Stunned!"));
            break;

        case EFuriDamageResponse::KnockBack:
            // 4. 넉백: 애니메이션 및 물리적 날려버리기
            if (KnockBackMontage)
            {
                PlayAnimMontage(KnockBackMontage);
            }

            // 밀려날 방향 계산
            FVector PushDirection = FVector::ZeroVector;
            if (InstigatorActor)
            {
                // 공격자의 위치에서 내 위치를 바라보는 수평 방향 벡터
                PushDirection = GetActorLocation() - InstigatorActor->GetActorLocation();
                PushDirection.Z = 0.0f; 
                PushDirection.Normalize(); 
            }
            else
            {
                // 공격자가 없으면 내 뒤로 날아감
                PushDirection = -GetActorForwardVector();
            }

            // 최종 힘 계산 (넉백 변수는 헤더에 선언되어 있다고 가정)
            FVector FinalLaunchVelocity = (PushDirection * KnockBackPushForce) + FVector(0.f, 0.f, KnockBackUpForce);

            // 물리적으로 캐릭터를 날려버림 (관성 무시 true)
            LaunchCharacter(FinalLaunchVelocity, true, true);
            break;
    }


    // 최종 대미지 적용 (GAS Gameplay Effect)
    if (DamageSpec.IsValid())
    {
        AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());
        UE_LOG(LogTemp, Log, TEXT("Took Damage: %f"), DamageInfo.Amount);
    }
}