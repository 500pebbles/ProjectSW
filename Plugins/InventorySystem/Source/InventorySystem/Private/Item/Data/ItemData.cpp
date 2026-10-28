#include "Item/Data/ItemData.h"


void UItemData::Initialize(const FDataTableRowHandle& InRowHandle)
{
	ItemRowHandle = InRowHandle;
}

const FItemDataTable* UItemData::GetItemData() const
{
	return ItemRowHandle.GetRow<FItemDataTable>(TEXT("UITemData::GetItemData"));
}

FIntPoint UItemData::GetItemSize() const
{
	const FItemDataTable* ItemData = GetItemData();
	
	if (!ItemData)
	{
		return FIntPoint::ZeroValue;
	}
	
	return ItemData->ItemSize;
}