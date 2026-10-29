#include "Core/PMGRoute.h"
#include "Algo/Reverse.h"

namespace
{
	/** 방의 진입 비용 (설정 단위, 2-7절). 루트를 찾을 때는 입구/출구를 뺀 모든 방이 1칸 방이라 크기별로 나누지 않는다 */
	constexpr int32 RoomCost = 1;

	/** 코드 안에서 비용에 곱하는 배율 (2-7절 정수 비용 단위). 비용 1의 ±10% 지터를 정수로 나타내려고 100배 한다 */
	constexpr int32 RouteCostScale = 100;

	/** 지터 범위 (배율 100 기준 ±10 = ±10%) */
	constexpr int32 RouteCostJitter = 10;

	/** 다익스트라 힙에 넣는 항목: RoomId 방에 Cost로 도착할 수 있다 */
	struct FPathHeapItem
	{
		int64 Cost = 0;
		int32 RoomId = INDEX_NONE;

		/** 힙에서 먼저 꺼낼 항목: 비용이 작은 것, 비용이 같으면 방 ID가 작은 것 */
		bool operator<(const FPathHeapItem& Other) const
		{
			if (Cost != Other.Cost)
			{
				return Cost < Other.Cost;
			}
			return RoomId < Other.RoomId;
		}
	};

	/**
	 * 방마다 진입 비용을 만든다 (방 ID 순서로 난수를 하나씩 쓴다).
	 * PenaltyRoomIds에 든 방은 RouteCostPenalty, 나머지는 RoomCost.
	 * 각 비용에 (100 ± 10)을 곱한다: 100배 정수 단위 + ±10% 지터
	 */
	TArray<int64> MakeRoomEnterCosts(const FPMGLayout& Layout, const TArray<int32>& PenaltyRoomIds, int32 RouteCostPenalty, FRandomStream& Stream)
	{
		TArray<bool> bIsPenaltyRoom;
		bIsPenaltyRoom.Init(false, Layout.Rooms.Num());
		for (const int32 RoomId : PenaltyRoomIds)
		{
			bIsPenaltyRoom[RoomId] = true;
		}

		TArray<int64> RoomEnterCosts;
		RoomEnterCosts.SetNum(Layout.Rooms.Num());
		for (const FPMGRoom& Room : Layout.Rooms)
		{
			const int32 SettingCost = bIsPenaltyRoom[Room.Id] ? RouteCostPenalty : RoomCost;

			// 예: 비용 1 → 1 x (90 ~ 110) = 90 ~ 110. RouteCostPenalty 10 → 900 ~ 1100 (10 단위)
			// int64: 설정 값이 커도 (예: RouteCostPenalty 1억 x 110) 넘치지 않게
			const int32 ScaleWithJitter = RouteCostScale + Stream.RandRange(-RouteCostJitter, RouteCostJitter);
			RoomEnterCosts[Room.Id] = static_cast<int64>(SettingCost) * ScaleWithJitter;
		}
		return RoomEnterCosts;
	}
}

TArray<int32> PMG::FindCheapestPath(const FPMGLayout& Layout, const TArray<int32>& StartRoomIds, const TArray<int32>& TargetRoomIds,
	const TArray<int64>& RoomEnterCosts, const TArray<bool>& bPassableRooms)
{
	const int32 RoomCount = Layout.Rooms.Num();
	check(RoomEnterCosts.Num() == RoomCount);
	check(bPassableRooms.Num() == RoomCount);

	TArray<bool> bIsTarget;
	bIsTarget.Init(false, RoomCount);
	for (const int32 TargetRoomId : TargetRoomIds)
	{
		bIsTarget[TargetRoomId] = true;
	}

	// 방마다: 지금까지 찾은 가장 싼 도착 비용, 그 경로에서 바로 앞 방, 비용이 확정되었는지
	TArray<int64> BestCosts;
	TArray<int32> PreviousRoomIds;
	TArray<bool> bIsDone;
	BestCosts.Init(MAX_int64, RoomCount);
	PreviousRoomIds.Init(INDEX_NONE, RoomCount);
	bIsDone.Init(false, RoomCount);

	// TArray를 힙으로 쓴다. HeapPush/HeapPop은 operator<로 비교해 가장 작은 항목을 맨 앞에 둔다
	TArray<FPathHeapItem> Heap;
	for (const int32 StartRoomId : StartRoomIds)
	{
		if (BestCosts[StartRoomId] == 0)
		{
			continue;   // 같은 시작 방이 두 번 들어옴
		}
		BestCosts[StartRoomId] = 0;
		Heap.HeapPush(FPathHeapItem{ 0, StartRoomId });
	}

	while (!Heap.IsEmpty())
	{
		FPathHeapItem Item;
		Heap.HeapPop(Item);

		// 같은 방이 비용을 바꿔 가며 힙에 여러 번 들어갈 수 있다. 처음 꺼낸 것(가장 싼 것)만 쓴다
		if (bIsDone[Item.RoomId])
		{
			continue;
		}
		bIsDone[Item.RoomId] = true;

		// 힙에서 꺼낸 순서 = 비용 순서이므로, 처음 꺼낸 목표 방이 가장 싸게 도착하는 목표다
		if (bIsTarget[Item.RoomId])
		{
			// 앞 방을 따라 시작 방까지 거슬러 올라간 뒤 뒤집는다
			TArray<int32> Path;
			for (int32 RoomId = Item.RoomId; RoomId != INDEX_NONE; RoomId = PreviousRoomIds[RoomId])
			{
				Path.Add(RoomId);
			}
			Algo::Reverse(Path);
			return Path;
		}

		for (const int32 NeighborId : Layout.Rooms[Item.RoomId].Neighbors)
		{
			if (bIsDone[NeighborId] || !bPassableRooms[NeighborId])
			{
				continue;
			}

			// 더 싸게 도착하는 길을 찾았을 때만 바꾼다. 비용이 같으면 먼저 찾은 앞 방을 그대로 둔다
			const int64 NewCost = Item.Cost + RoomEnterCosts[NeighborId];
			if (NewCost < BestCosts[NeighborId])
			{
				BestCosts[NeighborId] = NewCost;
				PreviousRoomIds[NeighborId] = Item.RoomId;
				Heap.HeapPush(FPathHeapItem{ NewCost, NeighborId });
			}
		}
	}

	// 힙이 빌 때까지 목표 방을 꺼내지 못했다 → 도착할 수 없음
	return TArray<int32>();
}

bool PMG::FindRoutes(FPMGLayout& Layout, int32 RouteCostPenalty, FRandomStream& Stream, FString& OutError)
{
	checkf(Layout.Rooms.IsValidIndex(Layout.EntranceRoomId) && Layout.Rooms.IsValidIndex(Layout.ExitRoomId),
		TEXT("FindRoutes는 입구/출구를 정한 뒤(ChooseEntranceAndExit) 불러야 한다"));

	// 루트는 맵의 모든 방을 지나갈 수 있다
	TArray<bool> bPassableRooms;
	bPassableRooms.Init(true, Layout.Rooms.Num());
	const TArray<int32> StartRoomIds = { Layout.EntranceRoomId };
	const TArray<int32> TargetRoomIds = { Layout.ExitRoomId };

	// 1. 루트 1: 모든 방 비용이 같다 → 지나는 방이 가장 적은 길 중 하나 (비용이 같은 길 중 어느 것인지는 지터가 정함)
	const TArray<int64> PrimaryCosts = MakeRoomEnterCosts(Layout, TArray<int32>(), RouteCostPenalty, Stream);
	TArray<int32> PrimaryRoute = FindCheapestPath(Layout, StartRoomIds, TargetRoomIds, PrimaryCosts, bPassableRooms);
	if (PrimaryRoute.IsEmpty())
	{
		OutError = FString::Printf(TEXT("경로 없음: 입구방 %d에서 출구방 %d까지 갈 수 없음"), Layout.EntranceRoomId, Layout.ExitRoomId);
		return false;
	}

	// 2. 루트 2: 루트 1에 든 방은 RouteCostPenalty → 루트 1을 피해 돌아가는 길이 싸다
	const TArray<int64> SecondaryCosts = MakeRoomEnterCosts(Layout, PrimaryRoute, RouteCostPenalty, Stream);
	TArray<int32> SecondaryRoute = FindCheapestPath(Layout, StartRoomIds, TargetRoomIds, SecondaryCosts, bPassableRooms);
	if (SecondaryRoute.IsEmpty())
	{
		// 루트 1을 찾았으므로 모든 방을 지나갈 수 있는 지금은 일어나지 않는다 (방어 코드)
		OutError = FString::Printf(TEXT("경로 없음: 입구방 %d에서 출구방 %d까지 루트 2를 찾지 못함"), Layout.EntranceRoomId, Layout.ExitRoomId);
		return false;
	}
	if (SecondaryRoute == PrimaryRoute)
	{
		OutError = FString::Printf(TEXT("루트 2가 루트 1과 같음: 입구방 %d와 출구방 %d 사이에 루트 1을 피해 가는 길이 없음 (방 %d개)"),
			Layout.EntranceRoomId, Layout.ExitRoomId, PrimaryRoute.Num());
		return false;
	}

	// 루트 방 수가 TotalRoomCount를 넘는지는 루트 위 큰 방 합치기(PMG::MergeRouteBigRooms) 뒤에 검사한다. 합치면 방 수가 줄기 때문이다
	Layout.PrimaryRoute = MoveTemp(PrimaryRoute);
	Layout.SecondaryRoute = MoveTemp(SecondaryRoute);
	return true;
}
