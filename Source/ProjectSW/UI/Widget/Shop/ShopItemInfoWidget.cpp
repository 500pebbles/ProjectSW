// 


#include "ShopItemInfoWidget.h"

#include "Components/Image.h"
#include "Components/SlateWrapperTypes.h"
#include "Components/TextBlock.h"
#include "Item/Data/ItemData.h"

void UShopItemInfoWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UShopItemInfoWidget::SetShopItemInfo(UItemData* Item, const FDataTableRowHandle& CostItemRow, int32 CostItemCount)
{
	// 판매 아이템 정보
	InitItemTooltip(Item);
	
	if (nullptr == CostItemData)
	{
		CostItemData = NewObject<UItemData>(this);
	}
	CostItemData->Initialize(CostItemRow);
	
	if (UTexture2D* Icon = CostItemData->GetItemIcon())
	{
		Image_CostItemIcon->SetBrushFromTexture(Icon);
	}
	Text_Cost->SetText(FText::Format(CostFormat, CostItemData->GetItemName(), CostItemCount));
}
