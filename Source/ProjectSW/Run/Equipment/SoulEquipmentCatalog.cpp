#include "Run/Equipment/SoulEquipmentCatalog.h"
#include "Run/Equipment/SoulEquipmentDefinition.h"
#include "Run/Equipment/SoulCostumeDefinition.h"
#include "Run/Core/SoulRunConfig.h"
#include "Run/Core/SoulRunGameComponent.h"
#include "Engine/Texture2D.h"
#include "Components/Image.h"

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

UTexture2D* USoulEquipmentCatalog::FindCostumeIcon(FName CostumeId) const
{
	const USoulCostumeDefinition* Costume = FindCostumeByName(CostumeId.ToString());
	return Costume ? Costume->Icon.LoadSynchronous() : nullptr;
}

UTexture2D* USoulEquipmentCatalog::FindWeaponIcon(FName WeaponId) const
{
	const USoulEquipmentDefinition* Weapon = WeaponId.IsNone() ? nullptr : FindWeaponByName(WeaponId.ToString());
	return Weapon ? Weapon->Icon.LoadSynchronous() : nullptr;
}

const USoulEquipmentCatalog* USoulEquipmentCatalog::GetRunCatalog(const UObject* WorldContextObject)
{
	const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(WorldContextObject);
	return Config ? Config->EquipmentCatalog.Get() : nullptr;
}

void USoulEquipmentCatalog::SetIcon(UImage* Image, UTexture2D* Icon)
{
	if (!Image)
		return;
	if (Icon)
	{
		Image->SetBrushFromTexture(Icon);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		Image->SetVisibility(ESlateVisibility::Collapsed);
	}
}
