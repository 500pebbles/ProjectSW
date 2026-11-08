#pragma once

#include "CoreMinimal.h"
#include "Item/Effect/ItemEffect.h"
#include "SoulItemEffect_EquipWeapon.generated.h"

/**
 * 무기 장착 효과. DA_Weapon_*(USoulEquipmentDefinition)의 Effects 안에 둔다.
 * 장착: 바깥 DA를 USoulEquipmentComponent::EquipWeapon으로. 해제: UnequipWeapon.
 * DA 공유 인스턴스라 상태를 저장하지 않는다. 팀 폰처럼 컴포넌트가 없으면 경고 1줄 후 나간다.
 */
UCLASS(meta = (DisplayName = "Soul Equip Weapon"))
class PROJECTSW_API USoulItemEffect_EquipWeapon : public UItemEffect
{
	GENERATED_BODY()

public:
	virtual void ActivateItemEffect_Implementation(AActor* User, UItemData* Item) override;
	virtual void DeactivateItemEffect_Implementation(AActor* User, UItemData* Item) override;
};
