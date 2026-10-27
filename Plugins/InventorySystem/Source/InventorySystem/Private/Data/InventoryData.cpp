#include "Data/InventoryData.h"

FInventoryDataArray::FInventoryDataArray()
{
	OwnerComponent = nullptr;
}

FInventoryDataArray::FInventoryDataArray(UActorComponent* InOwnerComponent)
{
	OwnerComponent = InOwnerComponent;
}

TArray<UItemData*> FInventoryDataArray::GetAllItems() const
{
}

UItemData* FInventoryDataArray::AddItemData(UItemComponent* InItemComponent)
{
}

UItemData* FInventoryDataArray::AddItemData(UItemData* InItemData)
{
}

void FInventoryDataArray::RemoveItemData(UItemData* InItemData)
{
}
