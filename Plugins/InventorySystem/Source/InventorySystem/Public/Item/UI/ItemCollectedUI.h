// 

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemCollectedUI.generated.h"

class USizeBox;
class UItemData;
class UImage;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UItemCollectedUI : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UImage* GetImageIcon() {return ItemIcon;}
	
	USizeBox* GetIconSizeBox() {return IconSizeBox;}
	
	void SetGridIndex(int32 Index) {GridIndex= Index;}
	int32 GetGridIndex() const {return GridIndex;}
	
	void SetGridPosition(FIntPoint Position) {GridPosition = Position;}
	FIntPoint GetGridPosition() const {return GridPosition;}
	
	void SetInventoryItem(UItemData* InInventoryItem);
	UItemData* GetInventoryItem() const {return InventoryItem.Get();}
	
private:
	// 이미지 아이콘
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> ItemIcon;
	
	UPROPERTY(Meta=(BindWidget))
	TObjectPtr<USizeBox> IconSizeBox; 
	
	// 배열상에서의 위치
	int32 GridIndex;
	
	// 그리드 좌표계에서의 위치
	FIntPoint GridPosition;
	
	// UI가 어떤 아이템의 UI인지 가리키기 위한 포인터
	TWeakObjectPtr<UItemData> InventoryItem;
};
