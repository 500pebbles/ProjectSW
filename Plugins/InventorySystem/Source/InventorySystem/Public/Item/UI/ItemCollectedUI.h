// 

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemCollectedUI.generated.h"

class UItemTooltip;
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
	
protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	
	UFUNCTION()
	void HandleDragEnded(UDragDropOperation* Operation);
	
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

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
	
	// UI 조작을 위한 섹션
public:
	FORCEINLINE void SetItemID(const FGuid& InID) {ItemID = InID;}
	FORCEINLINE const FGuid& GetItemID() const {return ItemID;}
	
private:
	FGuid ItemID;
	// 아이템 위젯 블루프린트 UI 좌표계에서 마우스가 눌린 위치
	FVector2D MousePressedLocalPosition = FVector2D::ZeroVector;
	
	UPROPERTY(EditDefaultsOnly, Category="ItemUI")
	TSubclassOf<UItemTooltip> ItemTooltipClass;
	
	UPROPERTY()
	TObjectPtr<UItemTooltip> ItemTooltip;
};
