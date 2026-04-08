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
	
	// 강제로 모든 설정을 코드로 고정 (블루프린트 설정보다 확실하게 확인하기 위함)
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Overlap); // 모든 채널에 오버랩 발생
	CollisionComp->SetGenerateOverlapEvents(true);
    
	CollisionComp->InitSphereRadius(100.f);
	
	
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
	SetReplicateMovement(true);
}

void AAuraBladeProjectile::Initialize(float InDamage, float InChargeRatio, FGameplayEffectSpecHandle InSpecHandle, FGameplayTag InHitTag)
{
	Damage = InDamage;
	ChargeRatio = InChargeRatio;
	DamageEffectSpecHandle = InSpecHandle;
	ImpactCueTag = InHitTag;
	
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
	
	UE_LOG(LogTemp, Error, TEXT("DEBUG: Overlap Detected with %s on Server!"), OtherActor ? *OtherActor->GetName() : TEXT("None"));
	//서버에서만 실행되도록 보장
	if (!HasAuthority() || !OtherActor)
		return;
	
	
	// 1. 자기 자신(발사체)이나 발사한 본인(Instigator)은 무시
	if (OtherActor == nullptr || OtherActor == GetInstigator())
	{
		return;
	}
	
	// if (HitActors.Contains(OtherActor))
	// {
	// 	return;
	// }

	// 🌟 [핵심] 첫 충돌 시 콜리전을 즉시 끕니다. 
	// 이렇게 하면 이 함수가 이번 프레임 이후로 다시는 호출되지 않습니다.
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// 2. 타겟의 ASC 가져오기 (GAS 라이브러리 사용)
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
    
	// 3. 타겟에게 ASC가 있고, 데미지 데이터(SpecHandle)가 유효한지 확인
	if (TargetASC && DamageEffectSpecHandle.Data.IsValid())
	{
		// 4. 발사한 사람(Source)의 ASC 가져오기
		UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator());

		if (SourceASC)
		{
			// 🌟 맞은 적 리스트에 추가
			// HitActors.Add(OtherActor);
			
			// 5. [핵심] 소스 ASC가 타겟 ASC에게 데미지 효과 적용!
			// *DamageEffectSpecHandle.Data.Get()으로 실제 데이터를 꺼내서 전달합니다.
			SourceASC->ApplyGameplayEffectSpecToTarget(*DamageEffectSpecHandle.Data.Get(), TargetASC);
            
			if (ImpactCueTag.IsValid())
			{
				FGameplayCueParameters HitParams;
				HitParams.Location = OtherActor->GetActorLocation(); // 맞은 대상의 위치
				// 필요하다면 타겟 액터를 직접 넘겨줄 수도 있습니다.
				HitParams.Instigator = GetInstigator();           // 공격자 정보 전달
				HitParams.EffectCauser = this;
            
				// "GameplayCue.P1.VFX.Hit" 태그를 사용하거나, 
				// 블루프린트에서 설정 가능한 태그 변수를 사용하세요.
				/// 🌟 핵심 변경: SourceASC가 아니라 TargetASC에서 실행!
				TargetASC->ExecuteGameplayCue(ImpactCueTag, HitParams);
			}
			UE_LOG(LogTemp, Warning, TEXT("🌙 명중 및 이펙트 실행!"));
			
		}
	}

	// 6. 투사체 파괴 (적을 관통하게 하고 싶다면 이 줄을 주석 처리하세요)
	// Destroy();
}
