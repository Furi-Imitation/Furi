# [Portfolio] Furi Replicant: Networked High-Speed Action
> **UE5 GAS 기반의 고밀도 전투 시스템 및 커스텀 데이터 파이프라인 구축**

---

## 🚀 프로젝트 개요 (Main Overview)

1. **핵심 컨셉**: 하드코어 액션 게임 'Furi'의 핵심 시스템을 모작하며, 고속 액션에 최적화된 전투 메커니즘과 실시간 네트워크 동기화를 구현한 프로젝트.
2. **개발 환경**: 
    - **Engine**: Unreal Engine 5.3
    - **Language**: C++ (Core), Blueprints (UI/VFX)
    - **Framework**: Gameplay Ability System (GAS)
    - **Networking**: Client-Server (Prediction & Custom Serialization)
3. **핵심 역할**: 고성능 액션 구현을 위한 GAS 프레임워크 확장, 네트워크 예측 기반 전투 시스템 설계, 데이터 주도형 아키텍처 구축.

---

## 🛠 문제 정의 및 해결 전략 (Why & How)

### 1. 문제 정의 (Why)
- **네트워크 데이터 유실**: GAS의 기본 컨텍스트가 프로젝트 고유의 데미지 데이터(`FFuriDamageInfo`)를 직렬화하지 못해 클라이언트-서버 간 판정 불일치 발생.
- **반응성 저하**: 네트워크 지연 발생 시 애니메이션이 멈추거나 콤보 입력이 씹히는 현상으로 인해 고속 액션의 조작감 훼손.
- **시스템 간 결합도**: 카메라 로직과 시네마틱 연출 로직이 강하게 결합되어 필살기 연출 중 화면 흔들림 발생.

### 2. 해결 전략 (How)
- **GAS 파이프라인 확장**: `FGameplayEffectContext`를 오버라이딩하여 커스텀 직렬화(`NetSerialize`) 로직을 직접 구현, 데이터 무결성 100% 확보.
- **예측 정책 적용**: `LocalPredicted` 정책과 `MontageSetNextSectionName`을 결합하여 지연 시간 중에도 클라이언트 사이드에서 즉각적인 피드백 제공.
- **상태 기반 제어**: `bIsCinematicMode` 상태 플래그를 도입하여 시스템 간 실행 로직을 물리적으로 격리(Decoupling).

### 3. 성과 (Result)
- **유지보수성 증대**: `DataAsset` 기반 설계로 기획 데이터 수정 시 코드 재빌드 없이 실시간 밸런싱 가능.
- **네트워크 최적화**: 비트 단위 직렬화를 통해 패킷 크기를 최소화하고 판정 오차 제거.
- **시각적 완성도**: 상태 관리를 통한 안정적인 시네마틱 뷰 전환으로 연출 퀄리티 향상.

---

## 💎 핵심 기술 상세 (Technical Deep Dive)

### 1. 커스텀 GAS 파이프라인 및 직렬화
- **개요**: 엔진 레벨의 GAS 컨텍스트를 확장하여 게임 고유의 데미지 판정 데이터를 네트워크 상에서 안전하게 전달하는 시스템.
- **설계 의도**: 단순 수치 전달을 넘어 패링 가능 여부, 가드 관통 등 복합적인 속성을 네트워크 패킷 수준에서 보장하기 위함.

```mermaid
sequenceDiagram
    participant C as Client (Instigator)
    participant S as Server
    Note over C: Create FFuriGameplayEffectContext
    Note over C: Set DamageInfo (Amount, Flags)
    C->>S: Send GameplayEffectSpec (NetSerialize)
    Note over S: Receive & Deserialize DamageInfo
    Note over S: Process Server-Side Hit Logic
    S-->>C: Replicate Result
```

- **핵심 성과**:
    - **네트워크 최적화**: `FArchive`를 활용한 선택적 직렬화로 불필요한 데이터 전송 차단.
    - **데이터 확장성**: `FFuriDamageInfo` 구조체 수정을 통해 새로운 공격 속성을 손쉽게 추가 가능.

### 2. 로컬 예측 기반 고속 콤보 시스템
- **개요**: 서버의 응답을 기다리지 않고 클라이언트에서 즉시 액션을 실행한 뒤 서버와 동기화하는 고성능 연격 시스템.
- **설계 의도**: 0.1초의 지연도 용납되지 않는 하드코어 액션 게임의 특성을 고려하여 **Local Predicted** 정책을 최우선으로 채택.

```mermaid
graph TD
    Input[User Input: Attack] --> Pred{Prediction Policy}
    Pred -->|Local| Client[Play Montage & Set Next Section]
    Client --> Event[Wait for Gameplay Event]
    Pred -->|Server| Auth[Validate Ability & Cost]
    Auth --> Sync[Replicate Montage & State]
    Event --> Hit[Perform Hit Check]
    Hit --> Effect[Apply Damage Effect]
```

- **핵심 성과**:
    - **반응성 확보**: 지연 시간(Ping) 100ms 환경에서도 끊김 없는 매끄러운 콤보 연계 달성.
    - **결합도 해소**: 애니메이션 몽타주와 GAS 태스크를 유기적으로 연결하여 코드 복잡도 감소.

### 3. 데이터 주도형(Data-Driven) 스킬 아키텍처
- **개요**: 모든 어빌리티의 수치(데미지, 코스트, 쿨타임)를 외부 데이터 에셋에서 동적으로 주입받는 구조.
- **설계 의도**: **OCP(개방-폐쇄 원칙)**를 준수하여, 로직 수정 없이 데이터만으로 새로운 스킬을 구성. `SetByCaller` 방식을 통해 고정된 GE 블루프린트의 한계를 극복하고 유연한 수치 제어 환경 구축.

```mermaid
graph TD
    subgraph DataLayer [Data Layer: Static Definitions]
        DA[UDataAsset: FFuriSkillData]
        TAGS[Gameplay Tags: Data.Damage.Amount, Data.Cost.Stamina]
    end

    subgraph LogicLayer [Logic Layer: Gameplay Ability]
        GA[UFuriGameplayAbilityBase]
        MakeSpec[MakeOutgoingGameplayEffectSpec]
    end

    subgraph Bridge [The Bridge: Runtime Injection]
        SBC[SetByCaller Magnitude]
    end

    subgraph EffectLayer [Effect Layer: Dynamic Specs]
        GE[UGameplayEffect Template]
        GES[FGameplayEffectSpec Instance]
    end

    subgraph StateLayer [State Layer: Character Attributes]
        ASC[Ability System Component]
        ATS[UAttributeSet: Health, Stamina, etc.]
    end

    DA -->|1. Provide Stats| GA
    GA -->|2. Load Template| GE
    GE -->|3. Create Instance| GES
    GA -->|4. Map Data to Tags| SBC
    SBC -->|5. Inject Values| GES
    GES -->|6. Apply to Target| ASC
    ASC -->|7. Update| ATS
```

- **핵심 성과**:
    - **생산성 향상**: 기획자가 엔진 에디터 상에서 직접 전투 밸런스를 조정할 수 있는 워크플로우 구축.
    - **유연성**: 동일한 `GameplayEffect` 클래스를 공유하면서도, 스킬 에셋에 따라 서로 다른 수치와 성격을 부여 가능.

### 4. 상태 기반 카메라 시스템 제어
- **개요**: 전투 중 거리 기반 자동 줌인/아웃 로직과 필살기 시네마틱 연출 간의 충돌을 방지하기 위한 상태 관리 시스템.

```mermaid
graph TD
    Tick[PlayerTick] --> Check{bIsCinematicMode?}
    Check -- False --> Std[UpdateStandardCamera]
    Check -- True --> Cine[Pause Auto-Update & Blend to Cine View]
    Std --> Cam[Dynamic ArmLength & FOV Interpolation]
    Cine --> End{Cinematic Finished?}
    End -- Yes --> Restore[FInterpTo Standard View]
```

---

## 📝 회고 (Retrospective)

본 프로젝트를 통해 단순히 기능을 구현하는 것을 넘어, **'엔진의 기본 프레임워크를 프로젝트의 목적에 맞게 확장하는 법'**을 깊이 있게 배웠습니다. 특히 네트워크 환경에서의 데이터 무결성과 조작 반응성 사이의 균형을 맞추는 과정에서 **커스텀 직렬화 및 예측 기술**의 중요성을 체감했습니다. 향후 대규모 멀티플레이어 환경에서도 안정적으로 동작하는 고성능 시스템을 설계할 수 있는 기술적 자신감을 얻었습니다.
