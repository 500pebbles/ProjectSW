// 


#include "Inventory/UI/Equipment/EquipSlotUI.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Inventory/Player/InventoryComponent.h"
#include "Inventory/Player/EquipmentComponent.h"
#include "Item/Data/ItemData.h"
#include "Item/UI/ItemCollectedUI.h"
#include "Item/UI/ItemDragDrop.h"
#include "Item/UI/ItemTooltip.h"

void UEquipSlotUI::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	if (SlotIcon && nullptr != DefaultSlotBusrh.GetResourceObject())
	{
		SlotIcon->SetBrush(DefaultSlotBusrh);
	}
}

void UEquipSlotUI::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	BindDelegates();
}

bool UEquipSlotUI::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	const UItemDragDrop* Operation = Cast<UItemDragDrop>(InOperation);
	if (!Operation)
	{
		return false;
	}
	
	ShowHighlight(CanEquip(Operation), Operation->ItemData);
	return true;
}

bool UEquipSlotUI::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	HideHighlight();
	
	const UItemDragDrop* Operation = Cast<UItemDragDrop>(InOperation);
	if (!Operation)
	{
		return false;
	}
	
	if (CanEquip(Operation) && EquipmentComponent.IsValid())
	{
		InventoryComponent->EquipItem(Operation->ItemID);
	}
	
	return true;
}

void UEquipSlotUI::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	
	HideHighlight();
}

FReply UEquipSlotUI::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	
	// 슬롯이 빈 경우 드래그 불가
	if (!EquippedItem || !SlotIcon)
	{
		return FReply::Unhandled();
	}
	
	// 슬롯이 아닌 아이콘을 기준으로 누른 위치를 구함
	const FGeometry IconGeometry = SlotIcon->GetCachedGeometry();
	const FVector2D IconLocalSize = IconGeometry.GetLocalSize();
	if (IconLocalSize.X > 0.0f && IconLocalSize.Y > 0.0f)
	{
		const FVector2D IconLocal = IconGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		PressedRatio = FVector2D(
			FMath::Clamp(IconLocal.X / IconLocalSize.X, 0.0f, 1.0f),
			FMath::Clamp(IconLocal.Y / IconLocalSize.Y, 0.0f, 1.0f));
	
	}
	else
	{
		PressedRatio = FVector2D(0.5f, 0.5f);
	}
	
	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}

void UEquipSlotUI::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
	
	if (!EquippedItem || !DragVisualClass)
	{
		return;
	}
	
	const FIntPoint ItemSize = EquippedItem->GetItemSize();
	const FVector2D VisualSize = FVector2D(ItemSize.X, ItemSize.Y) * TileSize;
	
	UItemDragDrop* Operation = NewObject<UItemDragDrop>();
	Operation->DragSource = EItemDragSource::Equipment;
	Operation->ItemData = EquippedItem;
	Operation->SourceEquipmentTag = SlotTag;
	
	Operation->MousePressedPosition = PressedRatio * VisualSize;
	
	// 나타난 아이템 UI 크기가 슬롯의 크기와 다르므로 나타난 아이템 UI 기준으로 마우스 위치를 맞춤
	Operation->Pivot = EDragPivot::TopLeft;
	Operation->Offset = -PressedRatio;
	
	UItemCollectedUI* Visual = CreateWidget<UItemCollectedUI>(GetOwningPlayer(), DragVisualClass);
	
	if (Visual)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(EquippedItem->GetItemIcon());
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = VisualSize;
		
		Visual->GetImageIcon()->SetBrush(Brush);
		Visual->GetIconSizeBox()->SetWidthOverride(VisualSize.X);
		Visual->GetIconSizeBox()->SetHeightOverride(VisualSize.Y);
		Visual->SetVisibility(ESlateVisibility::HitTestInvisible);
		Operation->DefaultDragVisual = Visual;
	}
	
	// 드래그 중에는 슬롯의 아이콘을 반투명하게 만듬
	SlotIcon->SetRenderOpacity(0.4f);
	Operation->OnDrop.AddDynamic(this, &UEquipSlotUI::HandleDragEnded);
	Operation->OnDragCancelled.AddDynamic(this, &UEquipSlotUI::HandleDragCancelled);
	
	OutOperation = Operation;
}

void UEquipSlotUI::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	
	if (!EquippedItem || !ItemTooltipClass)
	{
		return;
	}
	
	UpdateTooltip(EquippedItem.Get());
	
	if (!ItemTooltip->GetIsSet())
	{
		return;
	}
	
	ItemTooltip->SetVisibility(ESlateVisibility::HitTestInvisible);
	ItemTooltip->UpdateTooltipPosition(InGeometry);
}

void UEquipSlotUI::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	
	if (!ItemTooltip)
	{
		return;
	}
	
	ItemTooltip->SetVisibility(ESlateVisibility::Collapsed);
}

void UEquipSlotUI::HandleDragEnded(UDragDropOperation* Operation)
{
	// 착용 해제 실패 시 투명도 복원
	if (SlotIcon)
	{
		SlotIcon->SetRenderOpacity(1.0f);
	}
}

void UEquipSlotUI::HandleDragCancelled(UDragDropOperation* Operation)
{
	if (SlotIcon)
	{
		SlotIcon->SetRenderOpacity(1.0f);
	}
	
	// 월드에 버리는 경우
	if (InventoryComponent.IsValid())
	{
		InventoryComponent->DropEquippedItem(SlotTag);
	}
}

void UEquipSlotUI::UpdateTooltip(UItemData* Item)
{
	if (!ItemTooltip)
	{
		ItemTooltip = CreateWidget<UItemTooltip>(GetOwningPlayer(), ItemTooltipClass);
		ItemTooltip->SetAnchorsInViewport(FAnchors(0.f, 0.f));      // 점 앵커
		ItemTooltip->SetAlignmentInViewport(FVector2D(0.f, 0.f));
		ItemTooltip->AddToViewport(7);
	}
	
	ItemTooltip->InitItemTooltip(EquippedItem.Get());
}

void UEquipSlotUI::BindDelegates()
{
	const APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return;
	}
	APawn* PlayerPawn = PlayerController->GetPawn();
	
	if (!PlayerPawn)
	{
		return;
	}
	
	InventoryComponent = PlayerPawn->FindComponentByClass<UInventoryComponent>();
	if (!InventoryComponent.IsValid())
	{
		return;
	}
	
	EquipmentComponent = PlayerPawn->FindComponentByClass<UEquipmentComponent>();
	if (EquipmentComponent.IsValid())
	{
		EquipmentComponent->OnChangeEquipment.AddDynamic(this, &UEquipSlotUI::HandleEquipmentChanged);
		
		RefreshSlot(EquipmentComponent->GetEquippedItem(SlotTag));
	}
	else
	{
		RefreshSlot(nullptr);
	}
	
	
}

void UEquipSlotUI::RefreshSlot(UItemData* Item)
{
	EquippedItem = Item;
	
	if (!SlotIcon)
	{
		return;
	}
	
	if (!Item)
	{
		if (nullptr != DefaultSlotBusrh.GetResourceObject())
		{
			SlotIcon->SetBrush(DefaultSlotBusrh);
		}
		else
		{
			FSlateBrush Brush;
			SlotIcon->SetBrush(Brush);
			SlotIcon->SetVisibility(ESlateVisibility::Hidden);
		}
		return;
	}
	
	const FIntPoint ItemSize = Item->GetItemSize();
	FSlateBrush Brush;
	Brush.SetResourceObject(Item->GetItemIcon());
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = FVector2D(ItemSize.X, ItemSize.Y) * TileSize;
	
	SlotIcon->SetBrush(Brush);
	SlotIcon->SetRenderOpacity(1.0f);
	SlotIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
}

bool UEquipSlotUI::CanEquip(const UItemDragDrop* Operation) const
{
	if (!Operation || !EquipmentComponent.IsValid())
	{
		return false;
	}
	
	// 인벤토리에서 끌고온 슬롯만 장착 대상으로 판정
	if (Operation->DragSource != EItemDragSource::Inventory || !Operation->ItemData)
	{
		return false;
	}
	
	return EquipmentComponent->CanEquip(
		Operation->ItemData) && Operation->ItemData->GetEquipmentType().MatchesTagExact(SlotTag);
}

void UEquipSlotUI::ShowHighlight(bool bEquipable, UItemData* ItemData)
{
	if (!HighlightImage)
	{
		return;
	}
	
	if (!bEquipable && ItemData != EquippedItem)
	{
		HighlightImage->SetColorAndOpacity(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f));
	}
	else
	{
		HighlightImage->SetColorAndOpacity(FLinearColor(0.0f, 1.0f, 0.0f, 1.0f));
	}
	
	HighlightImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UEquipSlotUI::HideHighlight()
{
	if (HighlightImage)
	{
		HighlightImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UEquipSlotUI::HandleEquipmentChanged(FGameplayTag EquipmentTag, UItemData* Item)
{
	// 태그에 해당하는 아이템이 들어왔을 때만 처리
	if (!EquipmentTag.MatchesTagExact(SlotTag))
	{
		return;
	}
	
	RefreshSlot(Item);
	UpdateTooltip(Item);
}