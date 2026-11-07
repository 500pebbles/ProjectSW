// 

#pragma once

#include "CoreMinimal.h"
#include "Item/UI/ItemTooltip.h"
#include "Runtime/Engine/Classes/Engine/DataTable.h"
#include "ShopItemInfoWidget.generated.h"

class UTextBlock;
class UInventoryComponent;
class UItemData;
/**
 * 
 */
UCLASS(Abstract)
class PROJECTSW_API UShopItemInfoWidget : public UItemTooltip
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnInitialized() override;
	
	void SetShopItemInfo(UItemData* Item, const FDataTableRowHandle& CostItemRow, int32 CostItemCount);
	
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_CostItemIcon;
	
	UPROPERTY()
	TObjectPtr<UTextBlock> Text_Cost;
	
	// 아이템 교환 포멧 : 0 -> 교환에 필요한 아이템 이름, {1} -> 교환에 필요한 수량
	UPROPERTY(EditDefaultsOnly, Category="Shop")
	FText CostFormat = INVTEXT("{0} x{1}");
	
	// 교환에 필요한 아이템의 데이터를 읽고 임시로 저장
	UPROPERTY(Transient)
	TObjectPtr<UItemData> CostItemData;
};
