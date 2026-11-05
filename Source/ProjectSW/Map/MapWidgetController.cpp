


#include "MapWidgetController.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Map/SoulMapSubsystem.h"

static bool IsUsedRoom(const FPMGResultRoom& Room)
{
	return Room.bConnected && Room.VariantIndex != INDEX_NONE;
}


void UMapWidgetController::Init(APlayerController* InPlayerController)
{
	PlayerController = InPlayerController;
}

bool UMapWidgetController::GetUsedCellBounds(FIntRect& OutBounds) const
{
	bool bHasRoom = false;

	for (const FPMGResultRoom& Room : GetMapsubsystem()->GetMapResult().Rooms)
	{
		if (!IsUsedRoom(Room))
			continue;

		OutBounds.Min = bHasRoom ? OutBounds.Min.ComponentMin(Room.Bounds.Min) : Room.Bounds.Min;
		OutBounds.Max = bHasRoom ? OutBounds.Max.ComponentMax(Room.Bounds.Max) : Room.Bounds.Max;
		bHasRoom = true;
	}

	return bHasRoom;
}

TArray<FSoulMapRoomView> UMapWidgetController::GetRoomViews() const
{
	TArray<FSoulMapRoomView> Views;

	for (const FPMGResultRoom& Room : GetMapsubsystem()->GetMapResult().Rooms)
	{
		if (!IsUsedRoom(Room))
			continue;

		FSoulMapRoomView& View = Views.AddDefaulted_GetRef();
		View.RoomId = Room.Id;
		View.CellBounds = Room.Bounds;
	}

	return Views;
}

const USoulMapSubsystem* UMapWidgetController::GetMapsubsystem() const
{
	return PlayerController->GetWorld()->GetSubsystem<USoulMapSubsystem>();
}
