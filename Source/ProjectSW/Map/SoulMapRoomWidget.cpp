


#include "Map/SoulMapRoomWidget.h"
#include "Components/Button.h"
#include "Player/SoulPlayerController.h"
#include "Kismet/GameplayStatics.h"

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

	ASoulPlayerController* PC = GetOwningPlayer<ASoulPlayerController>();

	if (PC)
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeTime, FLinearColor::Black, false, false);
		}
		PC->ToggleMap();
	}

	UGameplayStatics::PlaySound2D(this, TeleportSound);
}
