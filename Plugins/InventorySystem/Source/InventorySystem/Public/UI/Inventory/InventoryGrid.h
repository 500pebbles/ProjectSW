// 

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryGrid.generated.h"

struct FInventoryDataEntry;
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
	
	void AddItemToGrid(const FInventoryDataEntry* InventoryItem);
	
	UItemCollectedUI* CreateCollectedItemUI(const FInventoryDataEntry* InventoryItem) const;
	
	// 그리드 좌표를 캔버스 좌표로 변환
	FVector2D GridToCanvasPosition(FIntPoint GridPosition) const;
	// 그리드 사이즈에 맞춰 사이즈 설정을 위한 함수
	FVector2D GridToCanvasSize(FIntPoint GridSize) const;
	
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
	
	UPROPERTY()
	TMap<FGuid, TObjectPtr<UItemCollectedUI>> CollectedItemWidgets;
};
