// GasCharacterBase.cpp

#include "GasCharacterBase.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Furi/GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"
#include "Furi/utils/FuriTypes.h"
#include "GameFramework/SpringArmComponent.h"

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
    
    // 1. UltSpringArm 생성
    UltSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("UltSpringArm"));
    // 캡슐(Root) 컴포넌트에 붙입니다. 보통 머리 위치 쯤인 소켓에 붙이기도 합니다.
    UltSpringArm->SetupAttachment(RootComponent); 
    
    // 기본 연출용 설정 (이 값들은 GA나 PC에서 실시간으로 변경 가능합니다.)
    UltSpringArm->TargetArmLength = 400.0f; // 적당히 가깝게
    UltSpringArm->SocketOffset = FVector(0.0f, 0.0f, 50.0f); // 살짝 위로
    
    // 🌟 중요: 연출 중 카메라가 벽이나 바닥을 뚫지 않도록 충돌 테스트를 켭니다.
    UltSpringArm->bDoCollisionTest = false; 
    
    // 캐릭터의 회전값에 따라 카메라가 휙휙 돌지 않도록 상속을 꺼둡니다. (고정 시점 연출용)
    UltSpringArm->bInheritPitch = false;
    UltSpringArm->bInheritYaw = true;
    UltSpringArm->bInheritRoll = false;

    // 2. UltCamera 생성
    UltCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("UltCamera"));
    // SpringArm 끝점(SocketName)에 붙입니다.
    UltCamera->SetupAttachment(UltSpringArm, USpringArmComponent::SocketName); 
    
    // 이 카메라는 PlayerController의 회전값(마우스 등)을 따르지 않게 설정합니다.
    UltCamera->bUsePawnControlRotation = false;
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
    // 1. ASC 및 속성 세트 유효성 검사
    if (!AbilitySystemComponent || !BasicAttributeSet) return;

    // 2. 사망 체크 (이미 체력이 0 이하라면 무시)
    if (BasicAttributeSet->GetHealth() <= 0.0f) return;

    // ---------------------------------------------------------
    // 🛡️ [방어 판정 1] 무적 (Invincible)
    // ---------------------------------------------------------
    bool bIsInvincible = AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Invincible")));
    if (bIsInvincible && !DamageInfo.bShouldDamageInvincible)
    {
        UE_LOG(LogTemp, Log, TEXT("[Furi] Damage Evaded: Character is Invincible."));
        return; // 데미지 및 리액션 없이 종료
    }

    // ---------------------------------------------------------
    // 🛡️ [방어 판정 2] 가드 (Blocking)
    // ---------------------------------------------------------
    bool bIsBlocking = AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Blocking")));
    if (bIsBlocking && DamageInfo.bCanBeBlocked)
    {
        UE_LOG(LogTemp, Log, TEXT("[Furi] Attack Blocked!"));
        
        // 가드 성공 시 데미지는 무시하고 전용 리액션 몽타주만 재생
        if (GuardReactionMontage)
        {
            PlayAnimMontage(GuardReactionMontage);
        }
        return; 
    }

    // ---------------------------------------------------------
    // 💥 [피격 확정] 데미지 적용 (Gameplay Effect)
    // ---------------------------------------------------------
    if (DamageSpec.IsValid())
    {
        // GE Spec을 통해 실제 Attribute(Health 등)를 깎습니다.
        AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());
    }

    // ---------------------------------------------------------
    // 🎬 [리액션 처리] 몽타주 및 상태(기절 등) 부여
    // ---------------------------------------------------------
    // 요청하신 대로 물리적 LaunchCharacter는 삭제했습니다. 
    // 모든 이동은 애니메이션의 'Root Motion'에 의존합니다.

    switch (DamageInfo.DamageResponse)
    {
        case EFuriDamageResponse::HitReaction:
            if (HitReactionMontage) PlayAnimMontage(HitReactionMontage);
            break;

        case EFuriDamageResponse::Stagger:
            if (StaggerMontage) PlayAnimMontage(StaggerMontage);
            break;

        case EFuriDamageResponse::Stun:
            // 🌟 [기절 특수 처리] GE를 통해 지속 시간만큼 State.Stun 태그 부여
          /*  if (StunGEClass)
            {
                FGameplayEffectContextHandle StunContext = AbilitySystemComponent->MakeEffectContext();
                FGameplayEffectSpecHandle StunSpec = AbilitySystemComponent->MakeOutgoingSpec(StunGEClass, 1.0f, StunContext);
                
                if (StunSpec.IsValid())
                {
                    // 전달받은 StunDuration을 GE의 지속 시간으로 설정
                    StunSpec.Data.Get()->SetDuration(DamageInfo.StunDuration, false);
                    AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*StunSpec.Data.Get());
                }
            }
        */
            // 기절 몽타주 재생 (태그 이벤트에 의해 중단될 수 있도록 설계)
            if (StunMontage) PlayAnimMontage(StunMontage);
            break;

        case EFuriDamageResponse::KnockBack:
            // 물리 힘 없이 넉백 몽타주만 재생 (Root Motion 활용)
            if (KnockBackMontage) PlayAnimMontage(KnockBackMontage);
            break;
    }
}