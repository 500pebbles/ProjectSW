#include "Run/Equipment/SoulItemEffect_EquipWeapon.h"
#include "Run/Equipment/SoulEquipmentComponent.h"
#include "Run/Equipment/SoulEquipmentDefinition.h"
#include "Utility/SoulLog.h"

void USoulItemEffect_EquipWeapon::ActivateItemEffect_Implementation(AActor* User, UItemData* Item)
{
	USoulEquipmentDefinition* Weapon = GetTypedOuter<USoulEquipmentDefinition>();
	USoulEquipmentComponent* Equipment = USoulEquipmentComponent::Find(User);
	if (!Equipment || !Weapon)
	{
		SW_LOG_WARNING("[WEAPON] %s: 장비 컴포넌트(%d)나 무기 정의(%d)가 없어 메시와 속성이 안 바뀐다", User ? *User->GetName() : TEXT("none"), Equipment != nullptr, Weapon != nullptr);
		return;
	}
	Equipment->EquipWeapon(Weapon);
}

void USoulItemEffect_EquipWeapon::DeactivateItemEffect_Implementation(AActor* User, UItemData* Item)
{
	if (USoulEquipmentComponent* Equipment = USoulEquipmentComponent::Find(User))
	{
		Equipment->UnequipWeapon(GetTypedOuter<USoulEquipmentDefinition>());
	}
}
