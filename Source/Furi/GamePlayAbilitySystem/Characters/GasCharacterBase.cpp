// GasCharacterBase.cpp

#include "GasCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Furi/GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"

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