#include "Core/PMGDoors.h"

namespace
{
	/** RoomA와 RoomB 사이에 문이 있는지 (RoomA, RoomB 순서와 무관) */
	bool HasDoorBetween(const FPMGLayout& Layout, int32 RoomA, int32 RoomB)
	{
		for (const FPMGDoor& Door : Layout.Doors)
		{
			const bool bSameOrder = Door.RoomA == RoomA && Door.RoomB == RoomB;
			const bool bReverseOrder = Door.RoomA == RoomB && Door.RoomB == RoomA;
			if (bSameOrder || bReverseOrder)
			{
				return true;
			}
		}
		return false;
	}
}

TArray<FPMGDoor> PMG::GetDoorCandidates(const FPMGLayout& Layout, int32 RoomA, int32 RoomB)
{
	checkf(Layout.Rooms.IsValidIndex(RoomA) && Layout.Rooms.IsValidIndex(RoomB), TEXT("GetDoorCandidates: 잘못된 방 ID (%d, %d)"), RoomA, RoomB);

	// 상하좌우 4방향 (PMG::BuildNeighbors와 같은 순서): +X, +Y, -X, -Y
	const FIntPoint Directions[] = { FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(-1, 0), FIntPoint(0, -1) };

	TArray<FPMGDoor> Candidates;
	const FIntRect& Bounds = Layout.Rooms[RoomA].Bounds;
	for (int32 Y = Bounds.Min.Y; Y < Bounds.Max.Y; ++Y)
	{
		for (int32 X = Bounds.Min.X; X < Bounds.Max.X; ++X)
		{
			const FIntPoint Cell(X, Y);
			for (const FIntPoint& Direction : Directions)
			{
				const FIntPoint OtherCell = Cell + Direction;
				if (Layout.IsInsideMap(OtherCell) && Layout.GetRoomIdAt(OtherCell) == RoomB)
				{
					FPMGDoor Door;
					Door.RoomA = RoomA;
					Door.RoomB = RoomB;
					Door.CellA = Cell;
					Door.CellB = OtherCell;
					Candidates.Add(Door);
				}
			}
		}
	}
	return Candidates;
}

void PMG::ConnectRooms(FPMGLayout& Layout, int32 RoomA, int32 RoomB, FRandomStream& Stream)
{
	if (HasDoorBetween(Layout, RoomA, RoomB))
	{
		return;   // 방 쌍마다 문은 하나 (예: 두 루트가 같은 방 쌍을 지남)
	}

	// 두 방 사이의 경계 후보 중 하나를 무작위로 고른다. 후보가 하나뿐이어도 난수를 쓴다 (난수 사용 순서가 후보 수에 따라 달라지지 않게)
	const TArray<FPMGDoor> Candidates = GetDoorCandidates(Layout, RoomA, RoomB);
	checkf(!Candidates.IsEmpty(), TEXT("ConnectRooms: 방 %d와 %d는 맞닿아 있지 않다"), RoomA, RoomB);
	Layout.Doors.Add(Candidates[Stream.RandRange(0, Candidates.Num() - 1)]);

	Layout.Rooms[RoomA].bConnected = true;
	Layout.Rooms[RoomB].bConnected = true;
}

void PMG::ConnectRoutes(FPMGLayout& Layout, FRandomStream& Stream)
{
	checkf(!Layout.PrimaryRoute.IsEmpty() && !Layout.SecondaryRoute.IsEmpty(), TEXT("ConnectRoutes는 루트를 찾은 뒤(FindRoutes) 불러야 한다"));

	// 루트에서 연속된 방 쌍마다 문을 만든다 (Route[0]과 Route[1], Route[1]과 Route[2], ...).
	// 루트 1을 먼저, 그다음 루트 2. 두 루트가 같은 방 쌍을 지나면 루트 1에서 만든 문을 같이 쓴다
	for (int32 Index = 1; Index < Layout.PrimaryRoute.Num(); ++Index)
	{
		ConnectRooms(Layout, Layout.PrimaryRoute[Index - 1], Layout.PrimaryRoute[Index], Stream);
	}
	for (int32 Index = 1; Index < Layout.SecondaryRoute.Num(); ++Index)
	{
		ConnectRooms(Layout, Layout.SecondaryRoute[Index - 1], Layout.SecondaryRoute[Index], Stream);
	}
}
