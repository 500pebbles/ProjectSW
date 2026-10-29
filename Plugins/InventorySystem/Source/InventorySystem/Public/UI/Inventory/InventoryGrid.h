// 

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryGrid.generated.h"

class UItemCollectedUI;
class UItemData;
class UCanvasPanel;
class UGridInventorySlot;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UInventoryGrid : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnInitialized() override;
	
private:
	void ConstructGrid();
	
	void BindDelegates();
	
	UFUNCTION()
	void AddItemToGrid(AActor* InventoryItem);
	
	UItemCollectedUI* CreateCollectedItemUI(UItemData* InventoryItem) const;
	
	// 인벤토리 UI 생성 (아래 UPROPERTY들은 WBP_InventoryGrid에서 선택)
private:
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UGridInventorySlot> GridSlotClass;
	
	UPROPERTY()
	TArray<TObjectPtr<UGridInventorySlot>> InventorySlots;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 Rows;
	
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 Cols;
	
	UPROPERTY(EditAnywhere, Category = "Inventory")
	float TileSize;
	
	// 습득한 아이템 UI반영
private:
	UPROPERTY(EditAnywhere, Category = "Item")
	TSubclassOf<UItemCollectedUI> CollectedItemClass;
};
