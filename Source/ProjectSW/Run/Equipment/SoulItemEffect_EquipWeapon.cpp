#include "Run/Equipment/SoulItemEffect_EquipWeapon.h"
#include "Run/Equipment/SoulEquipmentComponent.h"
#include "Run/Equipment/SoulEquipmentDefinition.h"
#include "Utility/SoulLog.h"

void USoulItemEffect_EquipWeapon::ActivateItemEffect_Implementation(AActor* User, UItemData* Item)
{
	USoulEquipmentDefinition* Weapon = GetTypedOuter<USoulEquipmentDefinition>();
	USoulEquipmentComponent* Equipment = USoulEquipmentComponent::Find(User);
	// 팀 맵은 컴포넌트가 있어도 카탈로그가 없어 접기 전과 같은 경고(0)로 나간다
	const bool bRunEquipment = Equipment && Equipment->GetCatalog();
	if (!bRunEquipment || !Weapon)
	{
		SW_LOG_WARNING("[WEAPON] %s: 장비 컴포넌트(%d)나 무기 정의(%d)가 없어 메시와 속성이 안 바뀐다", User ? *User->GetName() : TEXT("none"), bRunEquipment, Weapon != nullptr);
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
