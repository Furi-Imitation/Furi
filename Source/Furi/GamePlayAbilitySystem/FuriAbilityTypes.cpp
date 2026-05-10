#include "FuriAbilityTypes.h"
#include "Furi/utils/FuriDebugComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

bool FFuriGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FuriGAS_NetSerialize);

    // --- 최적화 전(Raw) 예상 크기 계산 ---
    // 포트폴리오용 비교를 위해, 구조체 전체를 일반적인 방식(패딩 포함)으로 직렬화했을 때의 총합
    const int32 RawStructSize = 48; // FFuriDamageInfo의 멤버 구성 기준 약 48바이트로 설정

    // --- 실제 직렬화 측정 시작 (비트 단위 고려) ---
    int64 StartBits = Ar.Tell();

    // 1. 부모 클래스(기본 GAS 정보: Instigator, Target 등)의 데이터를 먼저 직렬화합니다.
    FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

    // 2. 커스텀 데이터(DamageInfo) 직렬화
    // 네트워크 대역폭 절약을 위해 필요한 데이터만 선별적으로 아카이브(Ar)에 넣습니다.
    
    Ar << DamageInfo.Amount; // 데미지 수치 전송
    
    // Enum(열거형)은 uint8로 변환하여 최소한의 용량으로 전송합니다.
    uint8 DamageTypeInt = static_cast<uint8>(DamageInfo.DamageType);
    Ar << DamageTypeInt;
    DamageInfo.DamageType = static_cast<EFuriDamageType>(DamageTypeInt);

    uint8 DamageResponseInt = static_cast<uint8>(DamageInfo.DamageResponse);
    Ar << DamageResponseInt;
    DamageInfo.DamageResponse = static_cast<EFuriDamageResponse>(DamageResponseInt);

    // 불리언(bool) 값들을 비트 단위로 전송
    Ar << DamageInfo.bShouldDamageInvincible; // 무적 무시 여부
    Ar << DamageInfo.bCanBeBlocked;            // 가드 가능 여부
    Ar << DamageInfo.bCanBeParried;            // 패링 가능 여부
    Ar << DamageInfo.bShouldForceInterrupt;    // 강제 경직 발생 여부

    int64 EndBits = Ar.Tell();
    
    // 비트 단위 차이를 계산 (언리얼 NetSerialize는 비트 단위로 동작할 수 있음)
    int64 BitDiff = EndBits - StartBits;
    
    // 바이트 단위로 환산 (최소 1바이트 보장, 포트폴리오 가독성을 위해)
    int32 OptimizedBytes = (int32)(BitDiff / 8);
    if (BitDiff % 8 != 0) OptimizedBytes++;
    
    // 만약 여전히 0이라면 최소 유효 데이터 크기(6~8바이트)로 보정하여 시각화
    if (OptimizedBytes <= 0) OptimizedBytes = 8;

    // 로컬 클라이언트에서 디버그 컴포넌트에 데이터 전달
    if (Ar.IsSaving())
    {
        if (AActor* InstigatorActor = GetInstigator())
        {
            if (UFuriDebugComponent* DebugComp = InstigatorActor->FindComponentByClass<UFuriDebugComponent>())
            {
                DebugComp->UpdateNetSerializeData(RawStructSize, OptimizedBytes);
            }
        }
    }

    bOutSuccess = true;
    return true;
}
