// 


#include "UI/Inventory/InventoryGrid.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Item/Data/ItemData.h"
#include "Item/UI/ItemCollectedUI.h"
#include "Item/UI/ItemDragDrop.h"
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
		
		if (UInventoryComponent* InventoryComp = LocalPlayer->FindComponentByClass<UInventoryComponent>())
		{
			InventoryComponent = InventoryComp;
			InventoryComponent->OnInventoryItemAdded.AddUObject(this, &UInventoryGrid::AddItemToGrid);
			InventoryComponent->OnInventoryItemRemoved.AddDynamic(this, &UInventoryGrid::RemoveItemFromGrid);
			InventoryComponent->OnItemQuantityChanged.AddDynamic(this, &UInventoryGrid::UpdateItemQuantity);
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
	
	CollectedItemUI->SetItemID(InventoryItem->GetID());
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

void UInventoryGrid::RemoveItemFromGrid(FGuid ItemID)
{
	if (TObjectPtr<UItemCollectedUI>* RemovedItemIconWidget = CollectedItemWidgets.Find(ItemID))
	{
		if (*RemovedItemIconWidget)
		{
			(*RemovedItemIconWidget)->RemoveFromParent();
		}
		
		CollectedItemWidgets.Remove(ItemID);
	}
}

void UInventoryGrid::UpdateItemQuantity(FGuid ItemID, int32 NewQuantity)
{
	if (TObjectPtr<UItemCollectedUI>* Found = CollectedItemWidgets.Find(ItemID))
	{
		if (*Found)
		{
			// TODO: UI에 아이템 수량 조정
		}
	}
}

FVector2D UInventoryGrid::GridToCanvasPosition(FIntPoint GridPosition) const
{
	return FVector2D(GridPosition.X, GridPosition.Y) * TileSize;
}

FVector2D UInventoryGrid::GridToCanvasSize(FIntPoint GridSize) const
{
	return FVector2D(GridSize.X, GridSize.Y) * TileSize;
}

// 마우스를 때는 시점에 호출되는 함수. 여기서 true값을 반환하는게 있으면 넘어가고 false가 반환되면 OnDragCancelled API가 호출됨
// NativeOnDrop은 마우스 포인터 아래에 있는 위젯들에게 먼저 전달되고, 거기서 반환되는 boolean값으로 true false 여부 결정
bool UInventoryGrid::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	HidePlacementPreview();
	
	const UItemDragDrop* Operation = Cast<UItemDragDrop>(InOperation);
	if (!Operation || !InventoryComponent.IsValid())
	{
		return false;
	}
	
	const FIntPoint SlotPosition = ComputeTargetCell(InDragDropEvent, Operation);
	if (!InventoryComponent->MoveItemPosition(Operation->ItemID, SlotPosition))
	{
		return false;
	}
	
	// 데이터 이동 성공시 UI 위치 갱신
	if (UItemCollectedUI* ItemWidget = CollectedItemWidgets.FindRef(Operation->ItemID))
	{
		if (UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ItemWidget))
		{
			CanvasSlot->SetPosition(GridToCanvasPosition(SlotPosition));
		}
		ItemWidget->SetGridPosition(SlotPosition);
	}
	return true;
}

// 마우스를 드래그하는 동한 해당 위젯의 영역 내에 있으면 true를 반환 -> 영역 내에 마우스가 있어야 함수 호출됨
bool UInventoryGrid::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UItemDragDrop* Operation = Cast<UItemDragDrop>(InOperation);
	if (!Operation || !InventoryComponent.IsValid())
	{
		return false;
	}

	const FInventoryDataEntry* Entry = InventoryComponent->FindItemWithID(Operation->ItemID);
	if (!Entry)
	{
		return false;
	}

	// 그리드 안에서 ESC 등으로 취소돼도 하이라이트가 남지 않게 (중복 바인딩은 Unique로 방지)
	Operation->OnDragCancelled.AddUniqueDynamic(this, &UInventoryGrid::HandleDragCancelled);

	const FIntPoint Cell = ComputeTargetCell(InDragDropEvent, Operation);
	
	// 하이라이트가 그리드 밖으로 나가는 경우 인벤토리 그리드가 전체적으로 이동해서 본래 위치를 벗어나는 오류 방지
	// 이미지가 인벤토리의 그리드와 동일한 캔버스 패널에 위치해서 발생하는 것으로 추정됨
	// 그리드 밖으로 나갈 경우 하이라이트를 표시하지 않음
	const FIntPoint DataGrid = InventoryComponent->GetInventoryDataArray().GridSize;
	const FIntPoint EntrySize = Entry->GetOccupiedSize();
	if (Cell.X < 0 || Cell.Y < 0 || Cell.X + EntrySize.X > DataGrid.X || Cell.Y + EntrySize.Y > DataGrid.Y)
	{
		HidePlacementPreview();
		return true;
	}
	
	ShowPlacementPreview(Cell, Entry->GetOccupiedSize(), InventoryComponent->CheckSlotAvailable(Operation->ItemID, Cell));
	return true;
}

// 드래그 중인 마우스 포인터가 위젯 영역 밖으로 나갈 때 호출
void UInventoryGrid::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	
	HidePlacementPreview();
}

FIntPoint UInventoryGrid::ComputeTargetCell(const FDragDropEvent& InEvent, const UItemDragDrop* Operation) const
{
	const FVector2D CanvasLocal = CanvasPanel->GetCachedGeometry().AbsoluteToLocal(InEvent.GetScreenSpacePosition());
	const FVector2D ItemTopLeft = CanvasLocal - Operation->MousePressedPosition;
	
	// 가장 가까운 정수값으로 변환 후 반환
	return FIntPoint(FMath::RoundToInt(ItemTopLeft.X / TileSize), FMath::RoundToInt(ItemTopLeft.Y / TileSize));
}

void UInventoryGrid::ShowPlacementPreview(FIntPoint SlotLocation, FIntPoint Size, bool bCanPlace)
{
	if (!PlacementPreviewImage)
	{
		return;
	}
	
	if (UCanvasPanelSlot* PreviewSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(PlacementPreviewImage))
	{
		PreviewSlot->SetPosition(GridToCanvasPosition(SlotLocation));
		PreviewSlot->SetSize(GridToCanvasSize(Size));
	}
	
	PlacementPreviewImage->SetColorAndOpacity(
		bCanPlace ? FLinearColor(0.f, 1.f, 0.f, 0.35f): FLinearColor(1.f, 0.f, 0.f, 0.35f));
	
	PlacementPreviewImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UInventoryGrid::HidePlacementPreview()
{
	if (PlacementPreviewImage)
	{
		PlacementPreviewImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

// 마우스 드래그 중 위젯에서 true를 반환하지 않으면 실행 (인벤토리 위젯 밖에서 마우스 클릭을 때는 시점에 실행되는 것이 확인)
// 인벤토리 위젯 바깥에서 true를 반환하는 처리가 없다면 호출됨
void UInventoryGrid::HandleDragCancelled(UDragDropOperation* InOperation)
{
	HidePlacementPreview();
	
	// 아이템 삭제 처리
	if (!InventoryComponent.IsValid())
	{
		return;
	}
	
	UItemDragDrop* Operation = Cast<UItemDragDrop>(InOperation);
	if (!Operation)
	{
		return;
	}
	
	// 델리게이트를 이용해 아이템 제거 처리
	InventoryComponent->RemoveItem(Operation->ItemID);
}
