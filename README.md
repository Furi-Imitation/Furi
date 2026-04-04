# [Portfolio] Furi Replicant: High-Speed Networked Action
> **UE5 GAS 기반의 고성능 전투 시스템 및 다이내믹 카메라 프레임워크**

본 프로젝트는 고도의 컨트롤이 요구되는 액션 게임의 핵심 메커니즘을 **C++와 Gameplay Ability System(GAS)**으로 구현한 포트폴리오입니다. 특히 **네트워크 예측(Prediction)**과 **데이터 주도형 설계**를 통해 확장성과 안정성을 동시에 확보했습니다.

---

## 🛠 핵심 기술 스택
- **Engine**: Unreal Engine 5.3
- **Language**: C++ (Core Logic), Blueprints (UI & VFX)
- **Framework**: **Gameplay Ability System (GAS)**
- **Networking**: Client-side Prediction, Server Validation, RepNotify
- **Architecture**: Data-Driven (UDataAsset), Component-Based

---

## 💻 핵심 로직 분석 (Technical Deep Dive)

### 1. 지연 시간 없는 콤보 시스템 (Networked Combo System)
`GA_Attack` 어빌리티는 GAS의 **Local Predicted** 실행 정책을 사용하여 클라이언트에서 즉각적인 반응을 제공하며, 태그 기반의 콤보 윈도우를 통해 유연한 연격 로직을 구현했습니다.

```cpp
// GA_Attack.cpp: 콤보 입력 처리 및 예측(Prediction) 로직
void UGA_Attack::InputPressed(...) {
    // 콤보 입력 가능 시간(Window) 내에 입력이 들어왔는지 확인
    if (bComboWindowOpened && !bNextComboReserved && CurrentComboIndex < 3) {
        bNextComboReserved = true;
        
        // 타겟 방향으로 캐릭터를 자동 회전 (UX 향상)
        RotateTowardsClosestEnemy(MyAvatar, AttackRange);

        // 현재 섹션에서 다음 섹션으로의 몽타주 전이 예약
        FName CurrentSection = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex);
        FName NextSection = *FString::Printf(TEXT("Attack%d"), CurrentComboIndex + 1);
        MontageSetNextSectionName(CurrentSection, NextSection);
        CurrentComboIndex++;
    }
}
```
- **기술적 성과**: 클라이언트-서버 간의 몽타주 재생 위치를 동기화하고, `AbilityTask_WaitGameplayEvent`를 통해 애니메이션 특정 프레임에서 대미지 판정(`PerformHitCheck`)을 수행하여 시각적 연출과 로직의 일치성을 확보했습니다.

### 2. 다이내믹 보스전 카메라 (Dynamic Camera System)
`FuriPlayerController`에서 직접 구현한 카메라 로직은 플레이어와 보스 간의 거리를 실시간으로 계산하여 최적의 뷰를 제공합니다.

```cpp
// FuriPlayerController.cpp: 동적 거리 기반 줌인/아웃 로직
void AFuriPlayerController::UpdateStandardCamera(float DeltaTime) {
    // 1. 모든 캐릭터의 위치를 더해 중간 지점(Center) 계산
    FVector SumLocation = FVector::ZeroVector;
    for (AActor* Actor : Players) { SumLocation += Actor->GetActorLocation(); }
    FVector CenterTarget = SumLocation / Players.Num();

    // 2. 캐릭터 간 거리에 따라 목표 높이(TargetZ) 동적 설정
    float MaxDist = FVector::Dist(Players[0]->GetActorLocation(), Players[1]->GetActorLocation());
    float TargetZ = FMath::Clamp(MinCameraHeight + (MaxDist * CameraPadding), MinCameraHeight, MaxCameraHeight);

    // 3. FInterpTo를 사용한 부드러운 카메라 이동
    float NewZ = FMath::FInterpTo(CurrentCamLoc.Z, TargetZ, DeltaTime, ZoomInterpSpeed);
    
    // 4. 높이에 따른 X축 오프셋 보간 (멀어질수록 넓은 시야 확보)
    float HeightAlpha = FMath::GetMappedRangeValueClamped(FVector2D(MinHeight, MaxHeight), FVector2D(0, 1), NewZ);
    float DynamicOffsetX = FMath::Lerp(CloseOffset, FarOffset, HeightAlpha);

    FVector FinalTarget = CenterTarget + FVector(DynamicOffsetX, 0.f, 0.f);
    MainCameraActor->SetActorLocation(FinalTarget);
}
```
- **기술적 성과**: 단순한 `SpringArm` 사용이 아닌, 수동 위치 보간을 통해 **쿼터뷰와 사이드뷰의 장점**을 결합한 'Furi 스타일' 카메라를 C++로 완벽히 재현했습니다.

### 3. GAS 속성 시스템 및 네트워크 리플리케이션
`BasicAttributeSet`을 통해 캐릭터의 스탯을 정의하고, 네트워크 환경에서 안전하게 동기화합니다.

```cpp
// BasicAttributeSet.h: 특성 정의 및 리플리케이션 설정
UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
FGameplayAttributeData Health;

// PostGameplayEffectExecute: 수치 변경 후 클램핑(Clamping) 처리
void UBasicAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) {
    if (Data.EvaluatedData.Attribute == GetHealthAttribute()) {
        // 체력이 MaxHealth를 넘지 않도록, 0보다 작아지지 않도록 제한
        SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
    }
}
```
- **기술적 성과**: `PreAttributeChange`와 `PostGameplayEffectExecute`를 활용하여, 수치 변경 전후의 유효성 검사를 수행함으로써 네트워크 레이턴시 상황에서도 비정상적인 스탯 변화를 방지했습니다.

---

## 🚀 주요 문제 해결 경험 (Troubleshooting)

### [Issue] 네트워크 상황에서의 콤보 끊김 현상
- **현상**: 서버의 지연으로 인해 클라이언트에서 입력한 콤보가 제때 전달되지 않아 연격이 끊김.
- **해결**: GAS의 `NetExecutionPolicy`를 `LocalPredicted`로 설정하고, 클라이언트에서 즉시 몽타주 섹션을 변경하는 **예측 시스템**을 강화하여 지연 시간 환경에서도 매끄러운 콤보 발동 확인.

### [Issue] 시네마틱 연출 시 카메라 제어권 충돌
- **현상**: 필살기(`GA_Ultimate`) 연출 중 `PlayerController`의 자동 카메라 로직이 개입하여 화면이 흔들림.
- **해결**: `bIsCinematicMode` 플래그를 도입하여 연출 중에는 카메라 업데이트 로직을 일시 중단하고, 연출 종료 후 `FInterpTo`를 통해 원래의 전투 뷰로 부드럽게 복구하는 상태 관리 시스템 구축.

---

## 📂 프로젝트 아키텍처
- **Source/Furi/GamePlayAbilitySystem**: 콤보 공격, 패링, 대시 등 핵심 액션 로직 (GAS 기반)
- **Source/Furi/SSR**: AuraBlade 등 특수 스킬 및 발사체 물리 로직
- **Source/Furi/Weapons**: DataAsset 기반의 무기 스탯 및 몽타주 관리 시스템
- **Source/Furi/UI**: GAS Attribute와 연동되는 실시간 HUD 시스템

---

## 💡 종합 성과
본 프로젝트를 통해 **언리얼 엔진의 핵심 프레임워크인 GAS**를 실무 수준으로 다루는 역량을 증명했습니다. 단순히 기능을 만드는 것에 그치지 않고 **네트워크 환경에서의 동기화 오차 해결**과 **C++ 기반의 정교한 수학적 계산을 통한 카메라 제어** 등 기술적 디테일에 집중하여 프로젝트의 완성도를 높였습니다.
