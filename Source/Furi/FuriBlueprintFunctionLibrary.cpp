#include "FuriBlueprintFunctionLibrary.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "GamePlayAbilitySystem/Characters/GasCharacterBase.h"


AActor* UFuriBlueprintFunctionLibrary::FindClosestTarget(AActor* Searcher, float SearchRadius, const TArray<AActor*>& ActorsToIgnore)
{
    // 안전 검사
    if (!Searcher || !Searcher->GetWorld()) return nullptr;

    FVector SearchOrigin = Searcher->GetActorLocation();
    TArray<FOverlapResult> OverlapResults;
    
    // 자기 자신과, 인자로 받은 무시할 액터들을 필터링 설정에 넣습니다.
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Searcher);
    QueryParams.AddIgnoredActors(ActorsToIgnore);

    // 물리 엔진을 사용해 SearchRadius 반경 내의 모든 폰(Pawn)을 찾습니다.
    bool bHit = Searcher->GetWorld()->OverlapMultiByChannel(
        OverlapResults,
        SearchOrigin,
        FQuat::Identity,
        ECC_Pawn, // 타겟이 캐릭터/몬스터이므로 Pawn 채널 사용
        FCollisionShape::MakeSphere(SearchRadius),
        QueryParams
    );

    AActor* ClosestActor = nullptr;
    
    // 거리 비교를 위한 초기값 세팅 (반경보다 약간 큰 값의 제곱)
    // ※ 루트(Square Root) 연산은 무거우므로 거리의 제곱(DistSquared)으로 비교하는 것이 최적화의 기본입니다.
    float MinDistanceSq = FMath::Square(SearchRadius + 1.f);

    if (bHit)
    {
        // 🌟 2. 찾은 액터들을 순회하며 가장 가까운 녀석을 선별합니다.
        for (const FOverlapResult& Result : OverlapResults)
        {
            AActor* HitActor = Result.GetActor();
            if (HitActor && HitActor != Searcher)
            {
                // (선택) 아군/적군 판별이나, 이미 죽은 시체인지 확인하는 로직
                AGasCharacterBase* TargetChar = Cast<AGasCharacterBase>(HitActor);
                if (TargetChar /* && TargetChar->IsAlive() 같이 체력 검사 조건 추가 가능 */)
                {
                    // 나와 타겟 사이의 거리 제곱을 구함
                    float DistSq = FVector::DistSquared(SearchOrigin, HitActor->GetActorLocation());
                    
                    // 지금까지 찾은 애들보다 더 가깝다면 갱신!
                    if (DistSq < MinDistanceSq)
                    {
                        MinDistanceSq = DistSq;
                        ClosestActor = HitActor;
                    }
                }
            }
        }
    }

    return ClosestActor;
}

void UFuriBlueprintFunctionLibrary::CalculateTeleportTransform(AActor* Target, int32 StrikeIndex, float Distance, FVector& OutLocation, FRotator& OutRotation)
{
    // 타겟이 없으면 원점 반환 (안전 처리)
    if (!Target)
    {
        OutLocation = FVector::ZeroVector;
        OutRotation = FRotator::ZeroRotator;
        return;
    }

    // 타겟의 현재 위치와 방향 벡터(길이가 1인 화살표) 추출
    FVector TargetLoc = Target->GetActorLocation();
    FVector TargetForward = Target->GetActorForwardVector();
    FVector TargetRight = Target->GetActorRightVector();

    // 🌟 1. 타수(Index)별 위치 계산
    switch (StrikeIndex)
    {
    case 1:
        // 1타: 적의 등 뒤 (-Forward)
        OutLocation = TargetLoc - (TargetForward * Distance);
        break;

    case 2:
        // 2타: 적의 우측면 대각선 (앞으로 살짝 가고 오른쪽으로 이동)
        OutLocation = TargetLoc + (TargetForward * Distance * 0.5f) + (TargetRight * Distance);
        break;

    case 3:
        // 3타: 적의 좌측면 대각선 (앞으로 살짝 가고 왼쪽으로 이동)
        OutLocation = TargetLoc + (TargetForward * Distance * 0.5f) - (TargetRight * Distance);
        break;

    default:
        // 기본값은 등 뒤
        OutLocation = TargetLoc - (TargetForward * Distance);
        break;
    }

    // 높이(Z)는 타겟의 바닥 높이와 일치시킵니다. 
    // (공중 콤보를 원하시면 나중에 이 Z값을 높여주면 됩니다)
    OutLocation.Z = TargetLoc.Z;

    // 회전 계산: 내가 나타난 위치에서 적을 똑바로 쳐다보게 만듭니다.
    // MakeFromX는 "특정 방향을 정면(X축)으로 삼는 회전값"을 만들어주는 언리얼의 강력한 수학 함수입니다.
    FVector LookDirection = TargetLoc - OutLocation;
    LookDirection.Z = 0.f; // Z축(위아래)을 무시해야 쳐다볼 때 캐릭터가 위로 기울어지지 않습니다.
    OutRotation = FRotationMatrix::MakeFromX(LookDirection).Rotator();
}