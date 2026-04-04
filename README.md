# [Portfolio] Furi Replicant: Networked High-Speed Action
> **UE5 GAS 기반의 고밀도 전투 시스템 및 커스텀 데이터 파이프라인 구축**

본 프로젝트는 하드코어 액션 게임 **'Furi'**의 핵심 시스템을 모작하며, **Gameplay Ability System(GAS)**의 프레임워크를 심층 커스터마이징하여 고성능 액션 게임에 적합한 **데이터 주도형(Data-Driven) 전투 시스템**과 **실시간 네트워크 동기화**를 구현한 포트폴리오입니다.

---

## 🛠 Tech Stack & Networking
- **Engine**: Unreal Engine 5.3
- **Language**: C++ (Core Logic), Blueprints (UI & VFX Binding)
- **Framework**: **Gameplay Ability System (GAS)** (Network Ready)
- **Networking Model**: Client-Server (Prediction, Replication, Custom Serialization)
- **Architecture**: Data-Driven Design (UDataAsset), Component-Based

---

## 🎯 Key Technical Highlights (핵심 기술 역량)

### 1. 확장성 있는 전투 데이터 구조 및 아키텍처 (`FuriTypes.h`)
전투 중 발생하는 모든 판정과 무기 설정을 체계적으로 관리하기 위해 고유한 데이터 타입을 정의했습니다. 이를 통해 기획자가 코드 수정 없이 다양한 공격 속성과 무기를 구성할 수 있습니다.

- **EFuriDamageType & Response**: 근접, 투사체 등 공격 성격과 스태거, 기절 등 피격 반응을 세분화하여 정교한 액션 피드백 구현.
- **FFuriDamageInfo**: 데미지 양뿐만 아니라 가드/패링 가능 여부, 무적 무시 속성 등을 포함하여 GAS의 `EffectContext`에 실어 보내는 핵심 데이터 구조체.
- **FWeaponConfig & InputActionConfig**: 무기별 메쉬, 부착 소켓, 애니메이션 레이어(ABP) 및 입력 태그 매핑을 데이터화하여 실시간 무기 교체 시스템의 유연성 확보.

### 2. 커스텀 GAS 파이프라인 구축 (`AbilityTypes` & `Globals`)
표준 GAS의 한계를 극복하고 게임 고유의 데이터를 네트워크 상에서 안전하게 전달하기 위해 엔진 레벨의 기능을 오버라이딩했습니다.

- **FFuriGameplayEffectContext**: 기본 `FGameplayEffectContext`를 상속받아 `FFuriDamageInfo`를 포함하도록 확장. `NetSerialize`를 오버라이딩하여 커스텀 데이터를 비트 단위로 최적화하여 직렬화.
- **UFuriAbilitySystemGlobals**: 엔진이 `GameplayEffectContext`를 생성할 때 커스텀 컨텍스트를 할당하도록 `AllocGameplayEffectContext`를 가로채기(Override)하여 시스템 전반에 통합.

```cpp
// FuriAbilityTypes.cpp: 네트워크 최적화 직렬화 로직
bool FFuriGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) {
    FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess); // 부모 데이터 우선
    Ar << DamageInfo.Amount;            
    Ar << DamageInfo.bCanBeBlocked;     // 가드 가능 여부
    Ar << DamageInfo.bCanBeParried;     // 패링 가능 여부
    Ar << DamageInfo.bShouldForceInterrupt; // 강제 경직 여부
    bOutSuccess = true; return true;
}
```

### 3. 데이터 주도형 어빌리티 베이스 (`UFuriGameplayAbilityBase`)
모든 어빌리티가 외부 `DataAsset`으로부터 코스트와 쿨타임 정보를 동적으로 가져오도록 설계했습니다. `ApplyCost`, `CheckCost` 등을 오버라이딩하여 하드코딩을 배제했습니다.

```cpp
// UFuriGameplayAbilityBase.cpp: DataAsset 기반 코스트 주입
void UFuriGameplayAbilityBase::ApplyCost(...) const {
    if (UGameplayEffect* CostGE = GetCostGameplayEffect()) {
        FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CostGE->GetClass(), GetAbilityLevel());
        FFuriSkillData SkillData;
        if (GetCurrentSkillData(SkillData)) {
            // Data Asset의 수치를 SetByCaller로 동적 주입하여 유지보수성 극대화
            SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Cost.Stamina")), -SkillData.StaminaCost);
        }
        ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
    }
}
```

### 4. 네트워크 예측 기반 전투 및 카메라 시스템
- **Networked Combo System**: GAS의 **Local Predicted** 정책과 `AbilityTask_WaitGameplayEvent`를 결합하여 지연 시간 환경에서도 즉각적인 반응성을 제공.

```cpp
// GA_Attack.cpp: 콤보 입력 처리 및 예측(Prediction) 로직
void UGA_Attack::InputPressed(...) {
    if (bComboWindowOpened && !bNextComboReserved && CurrentComboIndex < 3) {
        bNextComboReserved = true;
        RotateTowardsClosestEnemy(MyAvatar, AttackRange); // 타겟 방향 자동 회전
        
        // 몽타주 섹션 전이 예약 (클라이언트 즉시 실행으로 반응성 확보)
        FName CurrentSection = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex);
        FName NextSection = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex + 1);
        MontageSetNextSectionName(CurrentSection, NextSection);
        CurrentComboIndex++;
    }
}
```

- **Dynamic Camera System**: `FuriPlayerController`에서 두 캐릭터 간의 거리를 실시간 계산하여 `ArmLength`와 `FOV`를 보간하는 커스텀 카메라 로직 구현.

```cpp
// FuriPlayerController.cpp: 동적 거리 기반 줌인/아웃 로직
void AFuriPlayerController::UpdateStandardCamera(float DeltaTime) {
    float MaxDist = FVector::Dist(Players[0]->GetActorLocation(), Players[1]->GetActorLocation());
    float TargetZ = FMath::Clamp(MinCameraHeight + (MaxDist * CameraPadding), MinCameraHeight, MaxCameraHeight);
    float NewZ = FMath::FInterpTo(CurrentCamLoc.Z, TargetZ, DeltaTime, ZoomInterpSpeed);
    
    float HeightAlpha = FMath::GetMappedRangeValueClamped(FVector2D(MinCameraHeight, MaxCameraHeight), FVector2D(0, 1), NewZ);
    float DynamicOffsetX = FMath::Lerp(CloseOffset, FarOffset, HeightAlpha);
    MainCameraActor->SetActorLocation(CenterTarget + FVector(DynamicOffsetX, 0.f, NewZ));
}
```

### 5. GAS 속성 시스템 및 리플리케이션 (`BasicAttributeSet`)
캐릭터의 체력, 스태미나 등을 서버 검증 하에 관리하며 실시간 동기화를 보장합니다.

```cpp
// BasicAttributeSet.cpp: 수치 변경 후 서버 측 클램핑 및 검증
void UBasicAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) {
    if (Data.EvaluatedData.Attribute == GetHealthAttribute()) {
        // 체력이 0~MaxHealth 범위를 벗어나지 않도록 보정
        SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
    }
}
```

---

## 🚀 Technical Challenges & Troubleshooting (문제 해결 경험)

### [Challenge] 네트워크 상에서의 커스텀 데미지 판정 데이터 유실
- **Problem**: 클라이언트에서 설정한 '방어 불가' 속성이 서버 판정 시 기본값으로 초기화되는 문제 발생.
- **Solution**: `FuriAbilityTypes`에서 `Duplicate()`(딥 카피)와 `NetSerialize()`를 직접 구현하여 네트워크 패킷 수준에서 데이터 무결성을 100% 확보.

### [Challenge] 네트워크 상황에서의 콤보 끊김 현상
- **현상**: 서버 지연 발생 시 애니메이션이 멈추고 콤보 연격이 끊기는 문제.
- **해결**: `LocalPredicted` 정책을 강화하여 클라이언트에서 몽타주 섹션 전이를 즉시 실행하도록 변경하여 지연 환경에서도 매끄러운 콤보 발동 확인.

### [Challenge] 시네마틱 연출 시 카메라 제어권 충돌
- **현상**: 필살기(`GA_Ultimate`) 연출 중 `PlayerController`의 자동 카메라 로직이 개입하여 화면 흔들림 발생.
- **해결**: `bIsCinematicMode` 플래그를 도입하여 연출 중에는 카메라 업데이트 로직을 일시 중단하고, 종료 후 `FInterpTo`를 통해 원래의 전투 뷰로 부드럽게 복구하는 상태 관리 시스템 구축.

---

## 📂 프로젝트 구조 및 핵심 데이터 (`FuriTypes.h`)

```cpp
// FuriTypes.h: 전역 사용 데이터 구조 정의
UENUM(BlueprintType)
enum class EFuriDamageType : uint8 { None, Melee, Projectile, Explosion, Environment };

UENUM(BlueprintType)
enum class EFuriDamageResponse : uint8 { None, HitReaction, Stagger, Stun, KnockBack };

USTRUCT(BlueprintType)
struct FFuriDamageInfo {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) float Amount = 0.f;
    UPROPERTY(EditAnywhere) EFuriDamageType DamageType;
    UPROPERTY(EditAnywhere) EFuriDamageResponse DamageResponse;
    UPROPERTY(EditAnywhere) bool bCanBeBlocked = false;   // 가드 가능 여부
    UPROPERTY(EditAnywhere) bool bCanBeParried = false;   // 패링 가능 여부
    UPROPERTY(EditAnywhere) bool bShouldForceInterrupt = false; // 강제 인터럽트 여부
};

USTRUCT(BlueprintType)
struct FWeaponConfig {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FGameplayTag WeaponTag;
    UPROPERTY(EditAnywhere) TArray<FWeaponSlotConfig> WeaponSlots;
    UPROPERTY(EditAnywhere) TSubclassOf<UAnimInstance> AnimLayerClass;
    UPROPERTY(EditAnywhere) float MaxWalkSpeed = 800.f;
};
```

- **GamePlayAbilitySystem**: 콤보 공격, 패링, 커스텀 GAS 파이프라인 (Types, Globals, Base)
- **SSR Module**: 투사체 물리 및 캐릭터 고유 스킬 시스템
- **Weapons & Input**: 데이터 에셋 기반의 무기 스탯 및 입력 매핑 관리 (`FuriTypes.h`)
- **UI System**: GAS Attribute와 실시간 연동되는 데이터 바인딩 HUD

---

## 💡 종합 성과
본 프로젝트를 통해 **GAS의 내부 동작 원리를 파악하고 엔진 레벨에서 시스템을 확장**하는 깊이 있는 기술력을 확보했습니다. 특히 **데이터 주도형 설계**를 통해 개발 효율성을 높이고, **커스텀 직렬화 및 예측 기술**을 통한 네트워크 최적화를 달성하여 고성능 액션 게임 개발에 필요한 핵심 역량을 증명했습니다.
