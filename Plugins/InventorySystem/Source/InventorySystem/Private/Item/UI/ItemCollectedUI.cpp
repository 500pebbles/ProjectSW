// 

#include "Item/UI/ItemCollectedUI.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Item/Data/ItemData.h"
#include "Item/UI/ItemDragDrop.h"
#include "Item/UI/ItemTooltip.h"

void UItemCollectedUI::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UItemCollectedUI::NativeDestruct()
{
	Super::NativeDestruct();
	
	ClearTooltip();
}

FReply UItemCollectedUI::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	//return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	
	// 드래그 감지는 누른 후 이동까지 해야하기 때문에 처음 누른 위치를 저장
	MousePressedLocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	
	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}

void UItemCollectedUI::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
	
	UItemDragDrop* Operation = NewObject<UItemDragDrop>();
	Operation->DragSource = EItemDragSource::Inventory;
	Operation->ItemData = GetInventoryItem();
	Operation->ItemID = ItemID;
	Operation->MousePressedPosition = MousePressedLocalPosition;
	Operation->Pivot = EDragPivot::MouseDown;
	
	// 아이템 위젯과 같은 클래스의 위젯을 만들고 아이콘, 위치, 크기 복사
	UItemCollectedUI* ClonedVisual = CreateWidget<UItemCollectedUI>(GetOwningPlayer(), GetClass());
	if (ClonedVisual)
	{
		const FSlateBrush& Brush = GetImageIcon()->GetBrush();
		ClonedVisual->GetImageIcon()->SetBrush(Brush);
		ClonedVisual->GetIconSizeBox()->SetWidthOverride(Brush.ImageSize.X);
		ClonedVisual->GetIconSizeBox()->SetHeightOverride(Brush.ImageSize.Y);
		ClonedVisual->SetVisibility(ESlateVisibility::HitTestInvisible);
		Operation->DefaultDragVisual = ClonedVisual;
	}
	
	// 원본은 반투명, 드롭 성공/취소 시 원복
	SetRenderOpacity(0.4f);
	Operation->OnDrop.AddDynamic(this, &UItemCollectedUI::HandleDragEnded);
	Operation->OnDragCancelled.AddDynamic(this, &UItemCollectedUI::HandleDragEnded);

	OutOperation = Operation;
}

void UItemCollectedUI::HandleDragEnded(UDragDropOperation* Operation)
{
	SetRenderOpacity(1.f);
}

void UItemCollectedUI::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	
	if (!ItemTooltipClass)
	{
		return;
	}
	
	if (!ItemTooltip)
	{
		ItemTooltip = CreateWidget<UItemTooltip>(GetOwningPlayer(), ItemTooltipClass);
		ItemTooltip->InitItemTooltip(InventoryItem.Get());
		ItemTooltip->SetAnchorsInViewport(FAnchors(0.f, 0.f));      // 점 앵커
		ItemTooltip->SetAlignmentInViewport(FVector2D(0.f, 0.f));
		ItemTooltip->AddToViewport(7);
	}
	
	if (!ItemTooltip->GetIsSet())
	{
		return;
	}
	
	ItemTooltip->SetVisibility(ESlateVisibility::HitTestInvisible);
	ItemTooltip->UpdateTooltipPosition(InGeometry);
}

void UItemCollectedUI::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	
	if (!ItemTooltip)
	{
		return;
	}
	
	ItemTooltip->SetVisibility(ESlateVisibility::Collapsed);
}

void UItemCollectedUI::ClearTooltip()
{
	if (ItemTooltip)
	{
		ItemTooltip->RemoveFromViewport();
		ItemTooltip = nullptr;
	}
}

void UItemCollectedUI::SetInventoryItem(UItemData* InInventoryItem)
{
	InventoryItem = InInventoryItem;
}
