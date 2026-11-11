// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "EquipSlotUI.generated.h"

class UItemTooltip;
class UImage;
class UEquipmentComponent;
class UInventoryComponent;
class UItemCollectedUI;
class UItemDragDrop;
class UItemData;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UEquipSlotUI : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativePreConstruct() override;
	
	virtual void NativeOnInitialized() override;
	
	// 장착을 위한 드롭 받는 부분
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	
	// 탈거를 위해 드래그를 시작하는 부분
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	
private:
	// 델리게이트 구독 처리를 일괄적으로 수행할 함수
	void BindDelegates();
	
	// 슬롯 이미지 갱신
	void RefreshSlot(UItemData* Item);
	
	// 해당 슬롯에 드래그로 장착하는 동작이 유효한 동작인지 확인
	bool CanEquip(const UItemDragDrop* Operation) const;
	
	void ShowHighlight(bool bEquipable, UItemData* ItemData);
	void HideHighlight();
	
	// 아이템 교체 시 수행될 델리게이트 함수
	UFUNCTION()
	void HandleEquipmentChanged(FGameplayTag EquipmentTag, UItemData* Item);
	
	// 드롭 성공 시 (아이템 이동 혹은 버리기 등 아이템 아이콘 UI로 드래그 드롭을 수행하는 모든 동작)
	UFUNCTION()
	void HandleDragEnded(UDragDropOperation* Operation);
	
	UFUNCTION()
	void HandleDragCancelled(UDragDropOperation* Operation);
	
	void UpdateTooltip(UItemData* Item);
	
private:
	// 해당 슬롯의 장비 슬롯 태그
	UPROPERTY(EditAnywhere, Category="Equipment", meta=(Categories="Equipment.Slot"))
	FGameplayTag SlotTag;
	
	// 장착 해제 시 나타날 아이템 UI
	UPROPERTY(EditAnywhere, Category="Equipment")
	TSubclassOf<UItemCollectedUI> DragVisualClass;
	
	// 드래그 비주얼의 한 칸 크기 -> InventoryGrid의 TileSize와 같은 값으로 맞출 것
	UPROPERTY(EditAnywhere, Category="Equipment")
	float TileSize = 54.0f;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SlotIcon;
	
	UPROPERTY(EditAnywhere, Category="Equipment")
	FSlateBrush DefaultSlotBusrh;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> HighlightImage;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UItemData> EquippedItem;
	
	TWeakObjectPtr<UInventoryComponent> InventoryComponent;
	TWeakObjectPtr<UEquipmentComponent> EquipmentComponent;
	
	// 마우스를 누른 이취를 아이콘 크기 대비 비율로 저장
	FVector2D PressedRatio = FVector2D(0.5f, 0.5f);
	
private:
	UPROPERTY(EditDefaultsOnly, Category="ItemUI")
	TSubclassOf<UItemTooltip> ItemTooltipClass;
	
	UPROPERTY()
	TObjectPtr<UItemTooltip> ItemTooltip;
};