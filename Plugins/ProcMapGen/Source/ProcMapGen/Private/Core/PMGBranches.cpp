#include "Core/PMGBranches.h"
#include "Core/PMGDoors.h"

namespace
{
	/** RoomId 방의 이웃 중 미사용(bConnected가 false) 방. 이웃 목록 순서(오름차순) 그대로 */
	TArray<int32> GetUnusedNeighborIds(const FPMGLayout& Layout, int32 RoomId)
	{
		TArray<int32> UnusedNeighborIds;
		for (const int32 NeighborId : Layout.Rooms[RoomId].Neighbors)
		{
			if (!Layout.Rooms[NeighborId].bConnected)
			{
				UnusedNeighborIds.Add(NeighborId);
			}
		}
		return UnusedNeighborIds;
	}
}

bool PMG::FillBranches(FPMGLayout& Layout, int32 TotalRoomCount, FRandomStream& Stream, FString& OutError)
{
	// 1. 붙일 방 수 (예산) = TotalRoomCount - 지금 연결된 방 수
	int32 ConnectedRoomCount = 0;
	for (const FPMGRoom& Room : Layout.Rooms)
	{
		if (Room.bConnected)
		{
			ConnectedRoomCount++;
		}
	}
	const int32 BranchRoomCount = TotalRoomCount - ConnectedRoomCount;
	checkf(BranchRoomCount >= 0, TEXT("FillBranches: 연결된 방 %d개 > TotalRoomCount %d (Phase 4 방 수 검사를 통과했다면 일어나지 않음)"), ConnectedRoomCount, TotalRoomCount);

	// 2. 루트 방 목록 (ID 오름차순)과, 루트 방에 맞닿은 미사용 방 = 분기로 붙일 수 있는 방의 수
	//    한 방이 루트 방 여럿과 맞닿을 수 있으므로 처음 표시할 때만 센다
	TArray<int32> RouteRoomIds;
	TArray<bool> bCanBeBranch;
	bCanBeBranch.Init(false, Layout.Rooms.Num());
	int32 BranchCandidateCount = 0;
	for (const FPMGRoom& Room : Layout.Rooms)
	{
		if (!Room.IsRouteRoom())
		{
			continue;
		}
		RouteRoomIds.Add(Room.Id);
		for (const int32 NeighborId : GetUnusedNeighborIds(Layout, Room.Id))
		{
			checkf(Layout.Rooms[NeighborId].Shape == EPMGRoomShape::Single, TEXT("FillBranches: 미사용 큰 방 %d가 있음 (루트 옆 큰 방을 먼저 붙여야 한다)"), NeighborId);
			if (!bCanBeBranch[NeighborId])
			{
				bCanBeBranch[NeighborId] = true;
				BranchCandidateCount++;
			}
		}
	}

	// 3. 시도 실패 검사. 루트 방은 바뀌지 않고, 한 번에 붙일 수 있는 방 하나가 연결되므로
	//    "붙이는 도중에 프런티어가 빔" = "붙일 수 있는 방이 예산보다 적음"이다. 그래서 Layout을 바꾸기 전에 미리 판단한다
	if (BranchCandidateCount < BranchRoomCount)
	{
		OutError = FString::Printf(TEXT("분기 채우기 자리 부족: 루트 방에 맞닿은 미사용 방 %d개 < 붙일 방 %d개 (TotalRoomCount %d - 연결된 방 %d)"),
			BranchCandidateCount, BranchRoomCount, TotalRoomCount, ConnectedRoomCount);
		return false;
	}

	// 4. 예산만큼 하나씩 붙인다
	for (int32 AddedCount = 0; AddedCount < BranchRoomCount; ++AddedCount)
	{
		// 프런티어 = 미사용 이웃이 있는 루트 방. 방을 붙일 때마다 바뀌므로 매번 다시 만든다
		TArray<int32> FrontierRoomIds;
		for (const int32 RouteRoomId : RouteRoomIds)
		{
			if (!GetUnusedNeighborIds(Layout, RouteRoomId).IsEmpty())
			{
				FrontierRoomIds.Add(RouteRoomId);
			}
		}
		checkf(!FrontierRoomIds.IsEmpty(), TEXT("FillBranches: 프런티어가 빔 (3번 검사를 통과했다면 일어나지 않음)"));

		// 프런티어 방 하나 → 그 방의 미사용 이웃 하나. 하나뿐이어도 난수를 쓴다 (난수 사용 횟수가 후보 수에 따라 달라지지 않게)
		const int32 RouteRoomId = FrontierRoomIds[Stream.RandRange(0, FrontierRoomIds.Num() - 1)];
		const TArray<int32> UnusedNeighborIds = GetUnusedNeighborIds(Layout, RouteRoomId);
		const int32 BranchRoomId = UnusedNeighborIds[Stream.RandRange(0, UnusedNeighborIds.Num() - 1)];

		ConnectRooms(Layout, RouteRoomId, BranchRoomId, Stream);
		Layout.Rooms[BranchRoomId].bExtra = true;
	}
	return true;
}
