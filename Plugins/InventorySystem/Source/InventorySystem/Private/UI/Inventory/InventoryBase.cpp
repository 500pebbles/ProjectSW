// 


#include "UI/Inventory/InventoryBase.h"

void UInventoryBase::NativeConstruct()
{
	Super::NativeConstruct();
	
	SetVisibility(ESlateVisibility::Collapsed); 
}

void UInventoryBase::ToggleInventoryUI()
{
	if (Visibility == ESlateVisibility::Collapsed)
	{
		SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}
