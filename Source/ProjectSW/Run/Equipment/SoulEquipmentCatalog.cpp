#include "Run/Equipment/SoulEquipmentCatalog.h"
#include "Run/Equipment/SoulEquipmentDefinition.h"
#include "Run/Equipment/SoulCostumeDefinition.h"

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

USoulCostumeDefinition* USoulEquipmentCatalog::FindCostumeByName(const FString& Name) const
{
	if (Name.IsEmpty() || Name == TEXT("None"))
		return nullptr;
	TArray<USoulCostumeDefinition*> All;
	All.Add(DefaultCostume);
	All.Append(Costumes);
	for (USoulCostumeDefinition* Costume : All)
	{
		if (Costume && (Costume->GetName().Equals(Name, ESearchCase::IgnoreCase) || Costume->GetName().EndsWith(Name, ESearchCase::IgnoreCase)))
			return Costume;
	}
	return nullptr;
}
