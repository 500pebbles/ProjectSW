// 


#include "Inventory/UI/Inventory/InventoryBase.h"

void UInventoryBase::NativeConstruct()
{
	Super::NativeConstruct();
	
	SetVisibility(ESlateVisibility::Collapsed);
}

void UInventoryBase::ToggleInventoryUI()
{
	if (GetVisibility() == ESlateVisibility::Collapsed)
	{
		SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}
