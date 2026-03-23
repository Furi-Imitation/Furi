#include "WeaponManagerComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UWeaponManagerComponent::UWeaponManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponManagerComponent::EquipWeapon(const FWeaponConfig& NewConfig)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	// 1. 기존에 장착된 무기가 있다면 모두 제거 (메모리 관리 및 중복 방지)
	for (auto MeshComp : EquippedWeaponMeshes)
	{
		if (MeshComp)
		{
			MeshComp->DestroyComponent();
		}
	}
	EquippedWeaponMeshes.Empty();

	// 2. 새로운 무기 설정(FWeaponConfig)에 따라 무기 생성 및 부착
	for (const FWeaponSlotConfig& Slot : NewConfig.WeaponSlots)
	{
		if (!Slot.WeaponMesh) continue;

		// 새로운 스태틱 메쉬 컴포넌트 생성
		UStaticMeshComponent* NewWeaponMesh = NewObject<UStaticMeshComponent>(OwnerCharacter);
		NewWeaponMesh->SetStaticMesh(Slot.WeaponMesh);
        
		NewWeaponMesh->SetRelativeScale3D(Slot.WeaponScale);
		// 캐릭터의 메쉬에 부착하기 위해 등록
		NewWeaponMesh->RegisterComponent();

		// 부착 규칙: 소켓 위치/회전에 딱 맞게 스냅(Snap)
		FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, true);		NewWeaponMesh->AttachToComponent(OwnerCharacter->GetMesh(), AttachRules, Slot.AttachSocketName);

		// 관리 리스트에 추가
		EquippedWeaponMeshes.Add(NewWeaponMesh);
	}

	// 3. 캐릭터 상태 업데이트 (이동 속도)
	if (OwnerCharacter->GetCharacterMovement())
	{
		OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = NewConfig.MaxWalkSpeed;
	}

	// 4. 애니메이션 레이어 변경 (무기별 전용 모션 적용)
	if (NewConfig.AnimLayerClass)
	{
		OwnerCharacter->GetMesh()->SetAnimInstanceClass(NewConfig.AnimLayerClass);
	}
}

