


#include "Map/SoulMapRoomWidget.h"
#include "Components/Button.h"
#include "Player/SoulPlayerController.h"

void USoulMapRoomWidget::InitRoomView(const FSoulMapRoomView& View)
{
	RoomId = View.RoomId;

	WaypointButton->SetVisibility(View.bHasWaypoint ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	WaypointButton->SetIsEnabled(View.State != ESoulMapRoomState::Current);
	ApplyRoomView(View);
}

void USoulMapRoomWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	WaypointButton->OnClicked.AddDynamic(this, &ThisClass::HandleWaypointClicked);
}

void USoulMapRoomWidget::HandleWaypointClicked()
{
	UMapWidgetController* MapController = Cast<UMapWidgetController>(WidgetController);
	if (!MapController || !MapController->RequestTeleport(RoomId))
		return;

	if (ASoulPlayerController* PC = GetOwningPlayer<ASoulPlayerController>())
		PC->ToggleMap();
}
