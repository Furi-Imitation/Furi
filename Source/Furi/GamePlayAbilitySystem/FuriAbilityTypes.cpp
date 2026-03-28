#include "FuriAbilityTypes.h"

bool FFuriGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
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

    bOutSuccess = true;
    return true;
}
