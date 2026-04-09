#include "SSRAuraBlade.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Furi/SSR/AuraBladeProjectile.h"
#include "Furi/FuriBlueprintFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "Furi/GamePlayAbilitySystem/FuriAbilityTypes.h"

USSRAuraBlade::USSRAuraBlade()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void USSRAuraBlade::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                    const FGameplayAbilityActivationInfo ActivationInfo,
                                    const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	// 🌟 추가: 버튼을 떼도 어빌리티가 취소되지 않도록 강제 설정
	bRetriggerInstancedAbility = false;

	if (ChargeMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, ChargeMontage);
		// 🌟 [추가] 몽타주 중단/취소 시 어빌리티 종료 처리
		MontageTask->OnInterrupted.AddDynamic(this, &USSRAuraBlade::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &USSRAuraBlade::OnMontageInterrupted);
		MontageTask->ReadyForActivation();
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}
	// 🔍 로그 추가: 서버/클라이언트 양쪽에서 찍히는지 확인
	UE_LOG(LogTemp, Warning, TEXT("ActivateAbility - IsServer: %d"), ASC->IsOwnerActorAuthoritative());
	if (ChargeCueTag.IsValid())
	{
		ASC->AddGameplayCue(ChargeCueTag);
	}
	
	// 🔍 서버에서 DelayTime이 얼마인지 확인!
	UE_LOG(LogTemp, Warning, TEXT("DelayTime Check - IsServer: %d, Value: %f"), 
		   ASC->IsOwnerActorAuthoritative(), DelayTime);

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, DelayTime);
	if (DelayTask)
	{
		DelayTask->OnFinish.AddDynamic(this, &USSRAuraBlade::OnDelayFinished);
		DelayTask->ReadyForActivation();
	}
	else
	{
		// EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void USSRAuraBlade::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogTemp, Error, TEXT("=== SERVER END ABILITY DETECTED ==="));
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USSRAuraBlade::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USSRAuraBlade::OnDelayFinished()
{
    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

    if (!Character || !ASC)
    {
       EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
       return;
    }

    // ========================================================================
    // 1. [시각 연출] GameplayCue 실행 (로컬 예측)
    // ========================================================================
    // Local Predicted 정책이므로, 이 부분은 클라이언트와 서버 양쪽에서 독립적으로 실행됩니다.
    // 클라이언트가 서버의 허락을 기다리지 않고 즉시 효과음을 재생하므로 조작감이 매우 부드럽습니다.
    if (ChargeCueTag.IsValid()) { ASC->RemoveGameplayCue(ChargeCueTag); }
    if (FireCueTag.IsValid()) { ASC->ExecuteGameplayCue(FireCueTag); }

    FGameplayEffectSpecHandle DamageSpecHandle;
    float FinalDamageAmount = 20.0f; // 안전장치 기본값

    // ========================================================================
    // 2. [데이터 준비] Data Asset 기반 대미지 동적 컨텍스트 조립
    // ========================================================================
    FFuriSkillData SkillData;
    if (GetCurrentSkillData(SkillData))
    {
       FFuriDamageInfo DamageInfo = SkillData.DamageInfo;
       FinalDamageAmount = DamageInfo.Amount;

       if (BaseDamageEffectClass)
       {
          // 대상에게 누가 대미지를 입혔는지 추적하기 위한 컨텍스트(Context) 생성
          FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
          ContextHandle.AddInstigator(Character, Character);

          // Furi 프로젝트 전용 커스텀 컨텍스트에 넉백, 스턴 등의 세부 DamageInfo 구조체를 심습니다.
          if (FFuriGameplayEffectContext* FuriContext = FFuriGameplayEffectContext::GetFuriContext(ContextHandle))
          {
             FuriContext->SetDamageInfo(DamageInfo);
          }

          DamageSpecHandle = MakeOutgoingGameplayEffectSpec(BaseDamageEffectClass, GetAbilityLevel());

          if (DamageSpecHandle.IsValid())
          {
             DamageSpecHandle.Data.Get()->SetContext(ContextHandle);
             
             // 🌟 [안전장치] 기획자가 Data Asset에 20을 적든 -20을 적든, 
             // 절댓값(Abs)을 씌우고 -를 붙여 무조건 체력이 깎이는 데미지(음수)로 강제 변환합니다.
             float SafeDamage = -FMath::Abs(FinalDamageAmount);
             DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(
                FGameplayTag::RequestGameplayTag(FName("Data.Damage.Amount")), SafeDamage);
          }
       }
       else
       {
           UE_LOG(LogTemp, Error, TEXT("[USSRAuraBlade] BaseDamageEffectClass가 None입니다! 블루프린트 할당 요망."));
       }
    }
    else
    {
       UE_LOG(LogTemp, Error, TEXT("[USSRAuraBlade] DataAsset 로드 실패! 블루프린트의 Ability Tags를 확인하세요."));
    }

    // ========================================================================
    // 3. [타겟팅 및 투사체 소환] 
    // ========================================================================
    
    // 가장 가까운 적을 찾아 그 방향으로 회전합니다. (클라이언트/서버 공통 연출)
    TArray<AActor*> ActorsToIgnore;
    AActor* ClosestTarget = UFuriBlueprintFunctionLibrary::FindClosestTarget(Character, 1500.0f, ActorsToIgnore);
    if (ClosestTarget)
    {
        FVector LookDir = ClosestTarget->GetActorLocation() - Character->GetActorLocation();
        LookDir.Z = 0.f;
        if (!LookDir.IsNearlyZero())
        {
            Character->SetActorRotation(LookDir.Rotation());
        }
    }

    // 오직 권한을 가진 "서버"에서만 진짜 물리 액터를 스폰!
    const bool bIsServer = GetOwningActorFromActorInfo()->HasAuthority();
    
    if (bIsServer)
    {
        if (ProjectileClass)
        {
           FVector ForwardVector = Character->GetActorForwardVector();
           // 캐릭터 앞쪽으로 100, 위쪽으로 50 떨어진 위치에서 스폰하여 캐릭터 몸통과 즉시 부딪히는 것을 방지
           FVector SpawnLocation = Character->GetActorLocation() + (ForwardVector * 100.f) + FVector(0.f, 0.f, 50.f);
           FRotator SpawnRotation = Character->GetActorRotation();

           FActorSpawnParameters SpawnParams;
           SpawnParams.Owner = Character;
           SpawnParams.Instigator = Character;
           SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

           // 오직 서버만 이 투사체를 스폰합니다. 
           // 투사체 액터 내부의 bReplicates = true 설정 덕분에 클라이언트 화면에도 자동으로 생성되어 날아갑니다.
           AAuraBladeProjectile* Projectile = GetWorld()->SpawnActor<AAuraBladeProjectile>(
              ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

           if (Projectile)
           {
              // Data Asset에서 가져온 실제 대미지와 완벽하게 조립된 Effect Spec을 투사체에 장전합니다.
              Projectile->Initialize(FMath::Abs(FinalDamageAmount), 1.0f, DamageSpecHandle, HitCueTag);
           }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[USSRAuraBlade] ProjectileClass가 None입니다! 블루프린트에서 투사체를 할당하세요."));
        }
    }

    // ========================================================================
    // 4. [스킬 종료] 클라이언트의 서버 암살 방지 동기화
    // ========================================================================
    // 네 번째 인자(bReplicateEndAbility)에 bIsServer를 넣은 것이 핵심입니다.
    // 클라이언트는 0.8초가 지나면 조용히 혼자 스킬을 종료합니다 (false).
    // 서버는 0.8초가 지나고 투사체를 무사히 스폰한 뒤, 모든 클라이언트에게 "스킬 진짜 끝났다!"라고 방송합니다 (true).
    // 이렇게 해야 클라이언트가 핑 차이로 미세하게 먼저 끝나서 서버의 투사체 스폰을 취소시켜버리는 버그를 막을 수 있습니다.
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bIsServer, false);
}

// void USSRAuraBlade::OnDelayFinished()
// {
// 	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
// 	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
//
// 	if (!Character || !ASC)
// 	{
// 		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
// 		return;
// 	}
//
// 	// 시각 효과는 양쪽 다 실행
// 	if (ChargeCueTag.IsValid()) { ASC->RemoveGameplayCue(ChargeCueTag); }
// 	if (FireCueTag.IsValid()) { ASC->ExecuteGameplayCue(FireCueTag); }
//
// 	// 🌟 [핵심] 아무런 조건문 없이 바로 Spawn 시도
// 	if (ProjectileClass)
// 	{
// 		FVector ForwardVector = Character->GetActorForwardVector();
// 		FVector SpawnLocation = Character->GetActorLocation() + (ForwardVector * 150.f) + FVector(0.f, 0.f, 50.f);
// 		FRotator SpawnRotation = Character->GetActorRotation();
//
// 		FActorSpawnParameters SpawnParams;
// 		SpawnParams.Owner = Character;
// 		SpawnParams.Instigator = Character;
// 		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//
// 		// 서버에서 실행될 때만 액터가 생성되고, 리플리케이션 설정에 의해 클라이언트로 복제됩니다.
// 		AAuraBladeProjectile* Projectile = GetWorld()->SpawnActor<AAuraBladeProjectile>(
// 			ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
//
// 		if (Projectile)
// 		{
// 			UE_LOG(LogTemp, Warning, TEXT("!!! PROJECTILE SPAWNED !!!"));
//             
// 			// 대미지 로직 (최소한의 안전장치)
// 			FGameplayEffectSpecHandle DamageSpecHandle = MakeOutgoingGameplayEffectSpec(BaseDamageEffectClass, GetAbilityLevel());
// 			Projectile->Initialize(20.0f, 1.0f, DamageSpecHandle, HitCueTag);
// 		}
// 	}
//
// 	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
// }
