// 

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryGrid.generated.h"

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
};
