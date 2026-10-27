// 


#include "UI/Inventory/InventoryGrid.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Inventory/GridInventorySlot.h"

void UInventoryGrid::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	ConstructGrid();
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
