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

void AGasCharacterBase::HandleDamageResponse(EFuriDamageResponse Response, AActor* Attacker)
{
    // 캐릭터가 이미 죽었거나 하는 등의 예외 처리가 필요하다면 여기서 체크
    if (BasicAttributeSet->GetHealth() <= 0.0f) return;

    switch (Response)
    {
    case EFuriDamageResponse::HitReaction:
        // 1. 일반 피격: 가볍게 움찔하는 애니메이션 재생
        if (HitReactionMontage)
        {
            PlayAnimMontage(HitReactionMontage);
        }
        break;

    case EFuriDamageResponse::Stagger:
        // 2. 경직: 더 크게 비틀거리는 애니메이션 재생
        if (StaggerMontage)
        {
            PlayAnimMontage(StaggerMontage);
        }
        break;

    case EFuriDamageResponse::Stun:
        // 3. 기절: (보통 애니메이션 재생보다는 GAS 태그 'State.Stunned'를 부여해서 
        // 일정 시간 동안 입력을 막고, 애님 그래프에서 루프 애니메이션을 틉니다)
        UE_LOG(LogTemp, Warning, TEXT("Character Stunned!"));
        break;

    case EFuriDamageResponse::KnockBack:
        // 4. 넉백: 🌟 대전 게임의 하이라이트! 물리적인 힘을 가해 날려버립니다.
        
        if (KnockBackMontage)
        {
            PlayAnimMontage(KnockBackMontage); // 공중에 뜨는 애니메이션 재생
        }

        // 밀려날 방향(PushDirection) 계산
        FVector PushDirection = FVector::ZeroVector;

        if (Attacker)
        {
            // 공격자의 위치에서 내 위치를 바라보는 방향 벡터를 구합니다. (나를 밀어내는 방향)
            PushDirection = GetActorLocation() - Attacker->GetActorLocation();
            
            // Z축(위아래) 영향력을 없애서 오직 수평 방향으로만 계산되게 합니다.
            PushDirection.Z = 0.0f;
            PushDirection.Normalize(); // 벡터의 길이를 1로 만들어 순수 '방향'만 남김
        }
        else
        {
            // 만약 공격자 정보가 없다면 (함정 등에 맞았을 때) 무조건 내 뒤쪽으로 날아감
            PushDirection = -GetActorForwardVector();
        }

        // 최종 힘 계산: 수평 방향 벡터에 밀어내는 힘을 곱하고, 수직으로 띄우는 힘을 더합니다.
        FVector FinalLaunchVelocity = (PushDirection * KnockBackPushForce) + FVector(0.f, 0.f, KnockBackUpForce);

        // 🌟 [함수: LaunchCharacter] 언리얼 캐릭터 이동의 핵심 물리 함수
        // 두 번째, 세 번째 인자인 XYOverride, ZOverride를 true로 주면
        // 현재 캐릭터가 걷고 있든 뛰고 있든 기존 관성을 무시하고 새 힘으로 확 날려버립니다!
        LaunchCharacter(FinalLaunchVelocity, true, true);
        
        break;
    }
}