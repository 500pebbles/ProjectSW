


#include "SoulMapWidget.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Player/SoulPlayerController.h"
#include "Map/MapWidgetController.h"
#include "Map/SoulMapRoomWidget.h"

void USoulMapWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	CloseButton->OnClicked.AddDynamic(this, &ThisClass::RequestClose);
}

FReply USoulMapWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Escape || Key == ToggleKey)
	{
		if (!InKeyEvent.IsRepeat())
			RequestClose();

		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply USoulMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return FReply::Handled().SetUserFocus(TakeWidget());
}

void USoulMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	const UMapWidgetController* MapController = Cast<UMapWidgetController>(WidgetController);

	if (!MapController || !RoomWidgetClass)
		return;

	FIntRect Used;

	if (!MapController->GetUsedCellBounds(Used))
		return;

	CellPx = FMath::Min3<double>(MapFrameSize.X / Used.Width(), MapFrameSize.Y / Used.Height(), MaxCellPx);
	UsedCenter = FVector2D(Used.Min + Used.Max) * 0.5;

	for (const FSoulMapRoomView& View : MapController->GetRoomViews())
	{
		USoulMapRoomWidget* RoomWidget = CreateWidget<USoulMapRoomWidget>(this, RoomWidgetClass);
		UCanvasPanelSlot* RoomSlot = RoomCanvas->AddChildToCanvas(RoomWidget);

		RoomSlot->SetPosition(CellToPx(FVector2D(View.CellBounds.Min)) + RoomGapPx);
		RoomSlot->SetSize(FVector2D(View.CellBounds.Width(), View.CellBounds.Height()) * CellPx - RoomGapPx * 2.0);

		RoomWidget->SetWidgetController(WidgetController);
		RoomWidget->ApplyRoomView(View);
	}
}

void USoulMapWidget::RequestClose()
{
	if (ASoulPlayerController* PC = GetOwningPlayer<ASoulPlayerController>())
		PC->ToggleMap();
}

FVector2D USoulMapWidget::CellToPx(const FVector2D& Cell) const
{
	return MapFrameSize * 0.5 + (Cell - UsedCenter) * CellPx;
}
