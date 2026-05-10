# Unreal Engine GAS Debug 포트폴리오 캡처 가이드 (Log & Stats 버전)

본 가이드는 별도의 UI 생성 없이 **출력 로그(Output Log)**와 **화면 디버그 메시지(On-Screen Message)**만으로 GAS의 핵심 동작을 증명하는 시나리오를 설명합니다.

---

## 1. 캡처 전 준비 사항

### A. 에디터 설정
1.  **Output Log 필터링**: 에디터 하단 'Output Log' 탭에서 검색어에 `GAS-Debug`를 입력하여 관련 로그만 모아봅니다.
2.  **지연 환경 시뮬레이션**: 
    *   콘솔 명령창(`~`)에 `Net PktLag=200` 입력 (또는 Play 버튼 옆 세팅 -> Common Network Settings -> Packet Lag 설정).
    *   이 설정은 Prediction 기능을 강조하기 위해 필수적입니다.

### B. 유용한 내장 명령 (stat)
콘솔 명령창에 다음을 입력하여 엔진 내장 디버그 정보를 활성화합니다:
*   `stat GameplayAbilities`: 현재 활성화된 어빌리티 정보 출력.
*   `stat GameplayTags`: 캐릭터가 보유한 태그 리스트 실시간 출력.

---

## 2. 포트폴리오용 캡처 시나리오

### 시나리오 1: 로컬 예측과 서버 확정 (Local Prediction)
*   **동작**: 지연(200ms) 환경에서 공격 버튼 연타.
*   **캡처 지점 (로그)**:
    1.  `[GAS-Debug] Ability: GA_Attack | Status: PREDICTED` 로그 발생.
    2.  잠시 후 `[GAS-Debug] Ability: GA_Attack | Status: CONFIRMED` 로그 발생.
*   **어필 포인트**: "네트워크 지연이 있는 환경에서도 유저가 즉각적인 반응을 느낄 수 있도록 Local Prediction을 적용하였으며, 서버로부터의 확정 과정을 로그로 검증함."

### 시나리오 2: NetSerialize 데이터 압축 (Network Optimization)
*   **동작**: 적을 타격하여 데미지 패킷 전송.
*   **캡처 지점 (화면)**:
    *   화면 우측 상단에 하늘색(Cyan) 메시지로 출력되는 `Net Pack: 48 -> 12 bytes (Saved 75.0%)` 캡처.
*   **어필 포인트**: "커스텀 NetSerialize를 구현하여 가변적인 데미지 정보를 비트 단위로 압축, 네트워크 대역폭을 75% 이상 절약함."

### 시나리오 3: 콤보 시스템 내부 로직 (Combo Validation)
*   **동작**: 공격 후 콤보 윈도우가 닫힐 때까지 대기했다가 클릭.
*   **캡처 지점 (로그 & 화면)**:
    *   화면 중앙의 빨간색 메시지 `Combo Fail: Window Closed` 캡처.
    *   로그의 `[GAS-Debug] Combo Failed: Window Closed` 기록.
*   **어필 포인트**: "입력 버퍼와 콤보 윈도우의 유효성을 디버깅하여 유저에게 정확한 액션 피드백을 제공함."

---

## 3. Unreal Insights 시각화 증명 (강력 추천)

1.  에디터 상단 `Tools` -> `Unreal Insights` 실행.
2.  공격 액션 수행 후 Insights의 **CPU Profiler** 탭 확인.
3.  `FuriGAS_AbilityActivated` 스코프가 타임라인 상에 나타난 구간을 확대 캡처.
*   **어필 포인트**: "함수 단위의 프로파일링 마커를 주입하여 병목 지점을 파악하고 성능을 관리함."

---
**Tip**: 로그 캡처 시, 클라이언트와 서버 로그를 동시에 띄워두고 같은 타임스탬프에서 예측과 승인이 일어나는 것을 보여주면 전문성이 더욱 강조됩니다.
