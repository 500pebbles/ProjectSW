#include "Run/Equipment/SoulEquipmentCatalog.h"
#include "Run/Equipment/SoulEquipmentDefinition.h"

USoulEquipmentDefinition* USoulEquipmentCatalog::FindWeaponByRow(FName RowName) const
{
	if (RowName.IsNone())
		return nullptr;
	for (USoulEquipmentDefinition* Weapon : Weapons)
	{
		if (Weapon && Weapon->ItemRow.RowName == RowName)
			return Weapon;
	}
	return nullptr;
}

USoulEquipmentDefinition* USoulEquipmentCatalog::FindWeaponByName(const FString& Name) const
{
	TArray<USoulEquipmentDefinition*> All;
	All.Add(DefaultWeapon);
	All.Append(Weapons);
	for (USoulEquipmentDefinition* Weapon : All)
	{
		if (Weapon && (Weapon->GetName().Equals(Name, ESearchCase::IgnoreCase) || Weapon->GetName().EndsWith(Name, ESearchCase::IgnoreCase)
			|| Weapon->ItemRow.RowName.ToString().Equals(Name, ESearchCase::IgnoreCase)))
			return Weapon;
	}
	return nullptr;
}
