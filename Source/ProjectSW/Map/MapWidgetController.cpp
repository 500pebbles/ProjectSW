


#include "MapWidgetController.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Map/SoulMapSubsystem.h"
#include "Map/SoulPackedLevelActor.h"

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

	for (const FPMGResultRoom& Room : GetMapSubsystem()->GetMapResult().Rooms)
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
	const USoulMapSubsystem* MapSubsystem = GetMapSubsystem();
	const FPMGResult& Result = MapSubsystem->GetMapResult();

	TSet<int32> AdjacentRooms;
	for (const FPMGResultDoor& Door : Result.Doors)
	{
		const ASoulPackedLevelActor* RoomA = MapSubsystem->FindRoom(Door.RoomA);
		const ASoulPackedLevelActor* RoomB = MapSubsystem->FindRoom(Door.RoomB);

		if (!RoomA || !RoomB)
			continue;

		if (RoomA->IsVisited())
			AdjacentRooms.Add(Door.RoomB);
		if (RoomB->IsVisited())
			AdjacentRooms.Add(Door.RoomA);
	}

	TArray<FSoulMapRoomView> Views;

	for (const FPMGResultRoom& Room : GetMapSubsystem()->GetMapResult().Rooms)
	{
		if (!IsUsedRoom(Room))
			continue;

		const ASoulPackedLevelActor* RoomActor = MapSubsystem->FindRoom(Room.Id);
		if (!RoomActor)
			continue;

		ESoulMapRoomState State;
		if (Room.Id == MapSubsystem->GetCurrentRoomId())
			State = ESoulMapRoomState::Current;
		else if (RoomActor->IsVisited())
			State = ESoulMapRoomState::Visited;
		else if (AdjacentRooms.Contains(Room.Id))
			State = ESoulMapRoomState::Adjacent;
		else
			continue;

		FSoulMapRoomView& View = Views.AddDefaulted_GetRef();
		View.RoomId = Room.Id;
		View.CellBounds = Room.Bounds;
		View.State = State;
		View.RoomType = Room.RoomType;
	}

	return Views;
}

TArray<FSoulMapDoorView> UMapWidgetController::GetDoorViews() const
{
	const USoulMapSubsystem* MapSubsystem = GetWorld()->GetSubsystem<USoulMapSubsystem>();

	TArray<FSoulMapDoorView> Views;

	for (const FPMGResultDoor& Door : MapSubsystem->GetMapResult().Doors)
	{
		const ASoulPackedLevelActor* RoomA = MapSubsystem->FindRoom(Door.RoomA);
		const ASoulPackedLevelActor* RoomB = MapSubsystem->FindRoom(Door.RoomB);

		if (!RoomA || !RoomB)
			continue;

		// 한쪽이라도 방문 했을대만 표시
		if (!RoomA->IsVisited() && !RoomB->IsVisited())
			continue;

		FSoulMapDoorView& View = Views.AddDefaulted_GetRef();

		// 이웃한 두 셀 중심의 중점 = 두 셀 사이 경계의 중점
		View.Cell = (FVector2D(Door.CellA) + FVector2D(Door.CellB) + FVector2D(1.0, 1.0)) * 0.5;
		View.bCrossesX = Door.CellA.X != Door.CellB.X;
	}

	return Views;
}

const USoulMapSubsystem* UMapWidgetController::GetMapSubsystem() const
{
	return PlayerController->GetWorld()->GetSubsystem<USoulMapSubsystem>();
}
