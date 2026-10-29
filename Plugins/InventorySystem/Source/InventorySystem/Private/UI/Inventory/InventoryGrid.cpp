// 


#include "UI/Inventory/InventoryGrid.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Item/ItemComponent.h"
#include "Item/Data/ItemData.h"
#include "Item/UI/ItemCollectedUI.h"
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
			CanvasSlot->SetSize(FVector2D(TileSize, TileSize));
			
			const FIntPoint TilePostion(ix, iy);
			// 타일에 따라 크기 조정을 위해 TileSize를 곱함
			CanvasSlot->SetPosition(TilePostion * TileSize);
			
			InventorySlots.Add(InventorySlot);
		}
	}
}

void UInventoryGrid::BindDelegates()
{
	if(APlayerController* LocalPlayerController = GetOwningPlayer())
	{
		APawn* LocalPlayer = LocalPlayerController->GetPawn();
		if (LocalPlayer)
		{
			TArray<UActorComponent*> InventoryInputComponent = LocalPlayer->GetComponentsByInterface(UInventoryInputInterface::StaticClass());
	
			for (UActorComponent* InventoryInput : InventoryInputComponent)
			{
				if (IInventoryInputInterface* InputInterface = Cast<IInventoryInputInterface>(InventoryInput))
				{
					InputInterface->GetOnCollectItem().AddDynamic(this, &UInventoryGrid::AddItemToGrid);
				}
			}
		}
	}
}

void UInventoryGrid::AddItemToGrid(AActor* InventoryItem)
{
	if (UItemComponent* InventoryItemComponent = InventoryItem->GetComponentByClass<UItemComponent>())
	{
		if (UItemData* InventoryItemData = InventoryItemComponent->GetItemData())
		{
			UItemCollectedUI* CollectedItemUI = CreateCollectedItemUI(InventoryItemData);
		}
	}
}

UItemCollectedUI* UInventoryGrid::CreateCollectedItemUI(UItemData* InventoryItem) const
{
	UItemCollectedUI* CollectedItem = CreateWidget<UItemCollectedUI>(GetOwningPlayer(), CollectedItemClass);
	
	if (!CollectedItem || !CollectedItemClass)
	{
		return nullptr;
	}
	
	CollectedItem->SetInventoryItem(InventoryItem);
	
	FSlateBrush Brush;
	Brush.SetResourceObject(InventoryItem->GetItemIcon());
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = FVector2D(TileSize * InventoryItem->GetItemSize().X, TileSize * InventoryItem->GetItemSize().Y);
	
	CollectedItem->GetImageIcon()->SetBrush(Brush);
	
	return CollectedItem;
}
