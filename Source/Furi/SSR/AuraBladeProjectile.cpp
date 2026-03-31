// Fill out your copyright notice in the Description page of Project Settings.


#include "AuraBladeProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"



// Sets default values
AAuraBladeProjectile::AAuraBladeProjectile()
{
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	RootComponent = CollisionComp;
	
	// NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComp"));
	// NiagaraComp->SetupAttachment(RootComponent);
	// 겹침 이벤트 설정 (검기는 보통 튕겨나가지 않고 뚫고 지나가거나 사라지므로 Overlap 사용)
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AAuraBladeProjectile::OnOverlap);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 3000.f; // 월아천충 속도!
	ProjectileMovement->MaxSpeed = 3000.f;
	
	// 중력 무시 (직선으로 날아감)
	ProjectileMovement->ProjectileGravityScale = 0.f;
	
	// 네트워크 복제 설정
	bReplicates = true;
	
}

void AAuraBladeProjectile::Initialize(float InDamage, float InChargeRatio, FGameplayEffectSpecHandle InSpechHandle)
{
	Damage = InDamage;
	ChargeRatio = InChargeRatio;
	DamageEffectSpecHandle = InSpechHandle;
	
	SetActorScale3D(FVector(1.0f + ChargeRatio));
}

// Called when the game starts or when spawned
void AAuraBladeProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAuraBladeProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAuraBladeProjectile::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 1. 자기 자신(발사체)이나 발사한 본인(Instigator)은 무시
	if (OtherActor == nullptr || OtherActor == GetInstigator())
	{
		return;
	}

	// 2. 타겟의 ASC 가져오기 (GAS 라이브러리 사용)
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
    
	// 3. 타겟에게 ASC가 있고, 데미지 데이터(SpecHandle)가 유효한지 확인
	if (TargetASC && DamageEffectSpecHandle.Data.IsValid())
	{
		// 4. 발사한 사람(Source)의 ASC 가져오기
		UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator());

		if (SourceASC)
		{
			// 5. [핵심] 소스 ASC가 타겟 ASC에게 데미지 효과 적용!
			// *DamageEffectSpecHandle.Data.Get()으로 실제 데이터를 꺼내서 전달합니다.
			SourceASC->ApplyGameplayEffectSpecToTarget(*DamageEffectSpecHandle.Data.Get(), TargetASC);
            
			UE_LOG(LogTemp, Warning, TEXT("🌙 월아천충 명중! 타겟: %s"), *OtherActor->GetName());
		}
	}

	// 6. 투사체 파괴 (적을 관통하게 하고 싶다면 이 줄을 주석 처리하세요)
	Destroy();
}
