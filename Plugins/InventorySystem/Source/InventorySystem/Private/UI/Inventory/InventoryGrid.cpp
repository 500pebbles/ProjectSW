// 


#include "UI/Inventory/InventoryGrid.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Item/Data/ItemData.h"
#include "Item/UI/ItemCollectedUI.h"
#include "Player/InventoryComponent.h"
#include "Player/InventoryInputComponent.h"
#include "UI/Inventory/GridInventorySlot.h"

void UInventoryGrid::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	ConstructGrid();
	
	BindDelegates();
}

void UInventoryGrid::ConstructGrid()
{
	// 슬롯 공간을 동적 할당
	InventorySlots.Reserve(Rows * Cols);
	
	for (int32 iy = 0; iy < Cols; ++iy )
	{
		for (int ix = 0; ix < Rows; ++ix)
		{
			UGridInventorySlot* InventorySlot = CreateWidget<UGridInventorySlot>(this, GridSlotClass);
			CanvasPanel->AddChild(InventorySlot);

			// 슬롯의 인덱스 설정
			InventorySlot->SetTileIndex(ix + iy * Cols);
			
			// 캔버스 패널에 AddChild를 하면 캔버스 패널 슬롯이 생성됨
			// 해당 캔버스 패널 슬롯을 이용해서 사이즈와 위치 설정 (사이즈 위치 설정을 위함)
			UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(InventorySlot);
			CanvasSlot->SetSize(GridToCanvasSize(FIntPoint(1, 1)));
			
			const FIntPoint TilePostion(ix, iy);
			// 타일에 따라 크기 조정을 위해 TileSize를 곱함
			CanvasSlot->SetPosition(GridToCanvasPosition(TilePostion));
			
			InventorySlots.Add(InventorySlot);
		}
	}
}

void UInventoryGrid::BindDelegates()
{
	if(APlayerController* LocalPlayerController = GetOwningPlayer())
	{
		APawn* LocalPlayer = LocalPlayerController->GetPawn();
		if (!LocalPlayer)
		{
			return;
		}
		
		TArray<UActorComponent*> InventoryInputComponent = LocalPlayer->GetComponentsByInterface(UInventoryInputInterface::StaticClass());
		
		if (UInventoryComponent* InventoryComponent = LocalPlayer->FindComponentByClass<UInventoryComponent>())
		{
			InventoryComponent->OnInventoryItemAdded.AddUObject(this, &UInventoryGrid::AddItemToGrid);
		}
	}
}

void UInventoryGrid::AddItemToGrid(const FInventoryDataEntry* InventoryItem)
{
	UItemCollectedUI* CollectedItemUI = CreateCollectedItemUI(InventoryItem);
	if (!CollectedItemUI)
	{
		return;
	}

	UCanvasPanelSlot* CanvasSlot = CanvasPanel->AddChildToCanvas(CollectedItemUI);
	if (!CanvasSlot)
	{
		return;
	}

	// 크기는 위젯 안의 SizeBox 오버라이드가 정하므로 AutoSize를 유지하고, 위치만 그리드 좌표로 지정
	// 앵커/정렬은 기본값(좌상단, (0,0))이라 위치가 곧 아이템의 좌상단이 된다
	CanvasSlot->SetAutoSize(true);
	CanvasSlot->SetPosition(GridToCanvasPosition(InventoryItem->GetGridPosition()));

	CollectedItemUI->SetGridPosition(InventoryItem->GetGridPosition());
	CollectedItemWidgets.Add(InventoryItem->GetID(), CollectedItemUI);
}

UItemCollectedUI* UInventoryGrid::CreateCollectedItemUI(const FInventoryDataEntry* InventoryItem) const
{
	UItemCollectedUI* CollectedItem = CreateWidget<UItemCollectedUI>(GetOwningPlayer(), CollectedItemClass);
	
	if (!CollectedItem || !CollectedItemClass)
	{
		return nullptr;
	}
	
	UItemData* ItemData = InventoryItem->GetItem();
	
	CollectedItem->SetInventoryItem(ItemData);
	
	FSlateBrush Brush;
	// Brush에 Texture2D형을 넣으면 UImage로 바뀜
	Brush.SetResourceObject(ItemData->GetItemIcon());
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = FVector2D(TileSize * ItemData->GetItemSize().X, TileSize * ItemData->GetItemSize().Y);
	
	CollectedItem->GetImageIcon()->SetBrush(Brush);
	CollectedItem->GetIconSizeBox()->SetWidthOverride(Brush.ImageSize.X);
	CollectedItem->GetIconSizeBox()->SetHeightOverride(Brush.ImageSize.Y);
	
	return CollectedItem;
}

FVector2D UInventoryGrid::GridToCanvasPosition(FIntPoint GridPosition) const
{
	return FVector2D(GridPosition.X, GridPosition.Y) * TileSize;
}

FVector2D UInventoryGrid::GridToCanvasSize(FIntPoint GridSize) const
{
	return FVector2D(GridSize.X, GridSize.Y) * TileSize;
}
