#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Furi/utils/FuriTypes.h"
#include "FuriAbilityTypes.generated.h"

/**
 * FFuriGameplayEffectContext
 * GAS의 기본 EffectContext를 상속받아 게임 고유의 데미지 정보(FFuriDamageInfo)를 담는 커스텀 구조체입니다.
 */
USTRUCT(BlueprintType)
struct FFuriGameplayEffectContext : public FGameplayEffectContext
{
    GENERATED_BODY()

public:
    /** 엔진 내부에서 이 구조체의 타입 정보를 식별하기 위해 사용됩니다. */
    virtual UScriptStruct* GetScriptStruct() const override
    {
       return StaticStruct();
    }

    /** * 컨텍스트 복제 함수 (Deep Copy)
     * GameplayEffect가 복사되거나 실행될 때 호출되며, 커스텀 데이터가 누락되지 않도록 합니다.
     */
    virtual FFuriGameplayEffectContext* Duplicate() const override
    {
       FFuriGameplayEffectContext* NewContext = new FFuriGameplayEffectContext();
       *NewContext = *this; // 기본 멤버 복사
       
       if (GetHitResult())
       {
          // 히트 결과(FHitResult)가 있다면 메모리 깊은 복사를 수행하여 데이터 무결성을 유지합니다.
          NewContext->AddHitResult(*GetHitResult(), true);
       }
       return NewContext;
    }

    /** 네트워크 직렬화: 서버-클라이언트 간에 데이터를 주고받는 규칙을 정의합니다. */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;

    /** * 핸들(FGameplayEffectContextHandle)로부터 커스텀 컨텍스트를 안전하게 가져오는 정적 도우미 함수입니다.
     * 캐스팅 실패 시 nullptr을 반환하여 런타임 에러를 방지합니다.
     */
    static FFuriGameplayEffectContext* GetFuriContext(FGameplayEffectContextHandle Handle)
    {
       FGameplayEffectContext* BaseContext = Handle.Get();
       if (BaseContext && BaseContext->GetScriptStruct()->IsChildOf(FFuriGameplayEffectContext::StaticStruct()))
       {
          return static_cast<FFuriGameplayEffectContext*>(BaseContext);
       }
       return nullptr;
    }

    // --- Getter / Setter: 외부(Ability나 AttributeSet)에서 데미지 정보를 수정/참조할 때 사용 ---
    void SetDamageInfo(const FFuriDamageInfo& InDamageInfo) { DamageInfo = InDamageInfo; }
    const FFuriDamageInfo& GetDamageInfo() const { return DamageInfo; }

protected:
    /** 실제 게임 로직에서 사용할 상세 데미지 데이터 (데미지 양, 타입, 방어 가능 여부 등) */
    UPROPERTY()
    FFuriDamageInfo DamageInfo;
};

/**
 * TStructOpsTypeTraits
 * 언리얼 엔진의 구조체 시스템에 이 구조체가 '커스텀 네트워크 직렬화'와 '복사 연산'을 지원함을 알립니다.
 */
template<>
struct TStructOpsTypeTraits<FFuriGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FFuriGameplayEffectContext>
{
    enum
    {
       WithNetSerializer = true, // NetSerialize 함수를 사용함
       WithCopy = true           // 복사 생성자/연산자를 사용함
    };
};
