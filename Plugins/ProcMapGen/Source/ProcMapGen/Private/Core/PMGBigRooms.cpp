#include "Core/PMGBigRooms.h"
#include "Core/PMGDoors.h"

namespace
{
	// ============================================================
	// 1. 셀의 루트 정보
	// ============================================================

	/** 셀이 루트 1/2의 몇 번째 방인지 (루트에 없으면 INDEX_NONE). 입구/출구를 뺀 모든 방이 1칸이라 셀 하나 = 방 하나 */
	struct FCellRouteInfo
	{
		int32 PrimaryIndex = INDEX_NONE;
		int32 SecondaryIndex = INDEX_NONE;

		bool IsOnPrimaryRoute() const { return PrimaryIndex != INDEX_NONE; }
		bool IsOnSecondaryRoute() const { return SecondaryIndex != INDEX_NONE; }
		bool IsOnRoute() const { return IsOnPrimaryRoute() || IsOnSecondaryRoute(); }

		/** 소속(루트 1만 / 루트 2만 / 두 루트 공통)이 같은지 */
		bool HasSameRoutes(const FCellRouteInfo& Other) const
		{
			return IsOnPrimaryRoute() == Other.IsOnPrimaryRoute() && IsOnSecondaryRoute() == Other.IsOnSecondaryRoute();
		}
	};

	/** 셀마다 루트 정보 (인덱스 = Y * Cols + X) */
	TArray<FCellRouteInfo> MakeCellRouteInfos(const FPMGLayout& Layout)
	{
		TArray<FCellRouteInfo> RouteInfoAtCell;
		RouteInfoAtCell.SetNum(Layout.Rows * Layout.Cols);
		for (int32 Index = 0; Index < Layout.PrimaryRoute.Num(); ++Index)
		{
			const FIntPoint Cell = Layout.Rooms[Layout.PrimaryRoute[Index]].Bounds.Min;
			RouteInfoAtCell[Cell.Y * Layout.Cols + Cell.X].PrimaryIndex = Index;
		}
		for (int32 Index = 0; Index < Layout.SecondaryRoute.Num(); ++Index)
		{
			const FIntPoint Cell = Layout.Rooms[Layout.SecondaryRoute[Index]].Bounds.Min;
			RouteInfoAtCell[Cell.Y * Layout.Cols + Cell.X].SecondaryIndex = Index;
		}
		return RouteInfoAtCell;
	}

	// ============================================================
	// 2. 합칠 자리 찾기 (2-7절 루트 위, 2-8절 루트 옆)
	// ============================================================

	/** 정수들을 정렬하면 빈틈 없이 이어지는지 (예: 5, 3, 4 → 3, 4, 5 → 연속). 루트에는 같은 방이 두 번 없으므로 중복은 없다 */
	bool AreConsecutive(TArray<int32> Indices)
	{
		Indices.Sort();
		return Indices.Last() - Indices[0] == Indices.Num() - 1;
	}

	/**
	 * Bounds 자리의 칸들을 합쳐 루트 위 큰 방을 만들 수 있는지.
	 * 입구/출구 칸이 없고, 루트 칸이 2개 이상이며, 루트 칸들의 소속이 같고, 각 루트에서 순서가 연속이어야 한다.
	 * 2칸 자리는 칸이 2개뿐이라 "루트 칸 2개 이상" = "두 칸 모두 루트 칸"이 된다.
	 * 이미 고른 자리와 겹치는지는 보지 않는다 (RemoveOverlappingCandidates가 후보 목록에서 지운다).
	 */
	bool CanMergeOnRoute(const FIntRect& Bounds, const FPMGLayout& Layout, const TArray<FCellRouteInfo>& RouteInfoAtCell)
	{
		TArray<FCellRouteInfo> RouteCells;
		for (int32 Y = Bounds.Min.Y; Y < Bounds.Max.Y; ++Y)
		{
			for (int32 X = Bounds.Min.X; X < Bounds.Max.X; ++X)
			{
				// 입구/출구는 크기가 에셋으로 정해져 있어 합치지 않는다. 큰 입구/출구도 CellToRoom으로 모든 칸이 걸러진다
				const int32 RoomId = Layout.GetRoomIdAt(FIntPoint(X, Y));
				if (RoomId == Layout.EntranceRoomId || RoomId == Layout.ExitRoomId)
				{
					return false;
				}
				const int32 CellIndex = Y * Layout.Cols + X;
				if (RouteInfoAtCell[CellIndex].IsOnRoute())
				{
					RouteCells.Add(RouteInfoAtCell[CellIndex]);
				}
			}
		}

		if (RouteCells.Num() < 2)
		{
			return false;   // 루트가 방을 관통하려면 루트 칸이 2개 이상 있어야 한다
		}

		TArray<int32> PrimaryIndices;
		TArray<int32> SecondaryIndices;
		for (const FCellRouteInfo& RouteCell : RouteCells)
		{
			if (!RouteCell.HasSameRoutes(RouteCells[0]))
			{
				return false;   // 소속이 다른 칸을 합치면 두 루트가 한 방에서 만난다
			}
			if (RouteCell.IsOnPrimaryRoute())
			{
				PrimaryIndices.Add(RouteCell.PrimaryIndex);
			}
			if (RouteCell.IsOnSecondaryRoute())
			{
				SecondaryIndices.Add(RouteCell.SecondaryIndex);
			}
		}

		// 루트에서 떨어진 칸을 합치면 루트가 같은 방에 두 번 들어간다
		const bool bPrimaryConsecutive = PrimaryIndices.IsEmpty() || AreConsecutive(PrimaryIndices);
		const bool bSecondaryConsecutive = SecondaryIndices.IsEmpty() || AreConsecutive(SecondaryIndices);
		return bPrimaryConsecutive && bSecondaryConsecutive;
	}

	/**
	 * Phase 6: Bounds 자리에 루트 옆 큰 방을 붙일 수 있는지.
	 * 칸이 모두 미사용 1칸 방이고, 그중 한 칸이라도 루트 방과 맞닿아야 한다 (추가방은 루트 방에만 붙인다).
	 * 칸마다 1칸 방이므로 "칸이 루트 방과 맞닿음" = "그 칸의 방의 이웃 중에 루트 방이 있음"이다.
	 */
	bool CanAttachBesideRoute(const FIntRect& Bounds, const FPMGLayout& Layout)
	{
		bool bTouchesRouteRoom = false;
		for (int32 Y = Bounds.Min.Y; Y < Bounds.Max.Y; ++Y)
		{
			for (int32 X = Bounds.Min.X; X < Bounds.Max.X; ++X)
			{
				const FPMGRoom& CellRoom = Layout.Rooms[Layout.GetRoomIdAt(FIntPoint(X, Y))];
				if (CellRoom.Shape != EPMGRoomShape::Single || CellRoom.bConnected)
				{
					return false;   // 이미 큰 방이거나 사용 중인 칸 (루트 방, 이미 붙인 추가방)
				}
				for (const int32 NeighborId : CellRoom.Neighbors)
				{
					if (Layout.Rooms[NeighborId].IsRouteRoom())
					{
						bTouchesRouteRoom = true;
					}
				}
			}
		}
		return bTouchesRouteRoom;
	}

	/** Width x Height 모양으로 합칠 수 있는 모든 자리 (왼쪽 위 셀이 위쪽 줄부터, 왼쪽부터). 합칠 수 있는지는 CanMerge(자리)로 정한다 */
	TArray<FIntRect> FindMergeCandidates(const FPMGLayout& Layout, int32 Width, int32 Height, TFunctionRef<bool(const FIntRect& Bounds)> CanMerge)
	{
		TArray<FIntRect> Candidates;
		for (int32 Y = 0; Y + Height <= Layout.Rows; ++Y)
		{
			for (int32 X = 0; X + Width <= Layout.Cols; ++X)
			{
				const FIntRect Bounds(X, Y, X + Width, Y + Height);
				if (CanMerge(Bounds))
				{
					Candidates.Add(Bounds);
				}
			}
		}
		return Candidates;
	}

	/** 후보 중 PickedBounds의 자리와 칸이 겹치는 것을 지운다 (RemoveAll은 남는 후보의 순서를 유지한다) */
	void RemoveOverlappingCandidates(TArray<FIntRect>& Candidates, const TArray<FIntRect>& PickedBounds)
	{
		Candidates.RemoveAll([&PickedBounds](const FIntRect& Candidate)
		{
			for (const FIntRect& Picked : PickedBounds)
			{
				if (PMG::DoRectsOverlap(Candidate, Picked))
				{
					return true;
				}
			}
			return false;
		});
	}

	// ============================================================
	// 3. 퍼지게 고르기 (best-candidate 샘플링)
	// ============================================================

	/**
	 * 후보 중 무작위로 SampleCount개를 뽑고, 그중 이미 놓인 방들과 가장 멀리 떨어진 후보의 인덱스를 고른다.
	 * SampleCount = 설정의 SpreadCandidateCount. 1이면 완전 무작위 선택과 같다.
	 */
	int32 PickSpreadCandidate(const TArray<FIntRect>& Candidates, const TArray<FVector2D>& PlacedCenters, int32 SampleCount, FRandomStream& Stream)
	{
		// 1) 서로 다른 후보 인덱스를 무작위로 뽑는다 (이미 뽑은 인덱스가 나오면 다시 뽑는다)
		const int32 FinalSampleCount = FMath::Min(FMath::Max(1, SampleCount), Candidates.Num());
		TArray<int32> SampledIndices;
		while (SampledIndices.Num() < FinalSampleCount)
		{
			const int32 Index = Stream.RandRange(0, Candidates.Num() - 1);
			if (!SampledIndices.Contains(Index))
			{
				SampledIndices.Add(Index);
			}
		}

		// 2) 아직 놓인 방이 없으면 첫 표본 (= 무작위)
		if (PlacedCenters.IsEmpty())
		{
			return SampledIndices[0];
		}

		// 3) 가장 가까운 방까지의 맨해튼 거리가 가장 먼 표본 (같으면 먼저 뽑힌 것)
		int32 BestIndex = SampledIndices[0];
		double BestDistance = -1.0;
		for (const int32 Index : SampledIndices)
		{
			const FVector2D Center = PMG::GetRectCenter(Candidates[Index]);
			double NearestDistance = TNumericLimits<double>::Max();
			for (const FVector2D& PlacedCenter : PlacedCenters)
			{
				const double Distance = FMath::Abs(Center.X - PlacedCenter.X) + FMath::Abs(Center.Y - PlacedCenter.Y);
				NearestDistance = FMath::Min(NearestDistance, Distance);
			}

			if (NearestDistance > BestDistance)
			{
				BestDistance = NearestDistance;
				BestIndex = Index;
			}
		}
		return BestIndex;
	}

	/**
	 * Candidates에서 Count개까지 골라 반환한다. 고를 때마다 고른 자리의 중심을 PlacedCenters에 더하고, 고른 자리와 겹치는 후보를 지운다.
	 * 후보가 먼저 떨어지면 거기서 멈춘다 (반환 배열의 Num()이 Count보다 작다).
	 */
	TArray<FIntRect> PickMergePlaces(TArray<FIntRect>& Candidates, int32 Count, int32 SampleCount, TArray<FVector2D>& PlacedCenters, FRandomStream& Stream)
	{
		TArray<FIntRect> PickedBounds;
		while (PickedBounds.Num() < Count && !Candidates.IsEmpty())
		{
			const FIntRect Picked = Candidates[PickSpreadCandidate(Candidates, PlacedCenters, SampleCount, Stream)];
			PickedBounds.Add(Picked);
			PlacedCenters.Add(PMG::GetRectCenter(Picked));
			RemoveOverlappingCandidates(Candidates, PickedBounds);
		}
		return PickedBounds;
	}

	/** 루트 1과 루트 2에 한 번이라도 들어간 방의 수 (두 루트에 다 있는 방은 한 번만 센다) */
	int32 CountRoomsOnRoutes(const FPMGLayout& Layout)
	{
		TArray<bool> bIsOnRoute;
		bIsOnRoute.Init(false, Layout.Rooms.Num());
		for (const int32 RoomId : Layout.PrimaryRoute)
		{
			bIsOnRoute[RoomId] = true;
		}
		for (const int32 RoomId : Layout.SecondaryRoute)
		{
			bIsOnRoute[RoomId] = true;
		}

		int32 Count = 0;
		for (const bool bRoomOnRoute : bIsOnRoute)
		{
			if (bRoomOnRoute)
			{
				Count++;
			}
		}
		return Count;
	}
}

void PMG::ChooseReservedBigRooms(const TArray<FPMGRoomSizeSet>& ReservedRoomSizeChoices, int32 AvailableQuadRoomCount, int32 AvailableDualRoomCount,
	FRandomStream& Stream, int32& OutReservedQuadRoomCount, int32& OutReservedDualRoomCount)
{
	OutReservedQuadRoomCount = 0;
	OutReservedDualRoomCount = 0;
	for (const FPMGRoomSizeSet& Sizes : ReservedRoomSizeChoices)
	{
		const bool bCanReserveQuad = Sizes.bQuad && OutReservedQuadRoomCount < AvailableQuadRoomCount;
		const bool bCanReserveDual = Sizes.bDual && OutReservedDualRoomCount < AvailableDualRoomCount;

		if (bCanReserveQuad && bCanReserveDual)
		{
			// 두 크기 모두 에셋이 있고 남아 있으면 무작위
			if (Stream.RandBool())
			{
				OutReservedQuadRoomCount++;
			}
			else
			{
				OutReservedDualRoomCount++;
			}
		}
		else if (bCanReserveQuad)
		{
			OutReservedQuadRoomCount++;
		}
		else if (bCanReserveDual)
		{
			OutReservedDualRoomCount++;
		}
		// 둘 다 없으면 예약하지 않는다. 이 타입의 대상이 모자라면 Phase 7 배정에서 시도 실패가 된다
	}
}

bool PMG::MergeRouteBigRooms(FPMGLayout& Layout, int32 QuadRoomCount, int32 DualRoomCount, int32 ReservedRoomCount, int32 TotalRoomCount,
	int32 SpreadCandidateCount, FRandomStream& Stream, int32& OutLeftoverQuadRoomCount, int32& OutLeftoverDualRoomCount, FString& OutError)
{
	checkf(!Layout.PrimaryRoute.IsEmpty() && !Layout.SecondaryRoute.IsEmpty(), TEXT("MergeRouteBigRooms는 루트를 찾은 뒤(FindRoutes) 불러야 한다"));

	for (const FPMGRoom& Room : Layout.Rooms)
	{
		const bool bEntranceOrExit = Room.Id == Layout.EntranceRoomId || Room.Id == Layout.ExitRoomId;
		checkf(bEntranceOrExit || Room.Shape == EPMGRoomShape::Single, TEXT("MergeRouteBigRooms: 입구/출구를 뺀 모든 방이 1칸이어야 한다"));
	}

	// 1. 셀마다 루트 정보
	const TArray<FCellRouteInfo> RouteInfoAtCell = MakeCellRouteInfos(Layout);

	// 2. 이미 놓인 방: 입구/출구. 큰 방이 입구/출구 바로 옆에 몰리지 않게 거리 계산에 넣는다
	TArray<FVector2D> PlacedCenters;
	PlacedCenters.Add(Layout.Rooms[Layout.EntranceRoomId].GetCenter());
	PlacedCenters.Add(Layout.Rooms[Layout.ExitRoomId].GetCenter());

	// 3. 4칸 먼저 고른다
	auto CanMerge = [&Layout, &RouteInfoAtCell](const FIntRect& Bounds)
	{
		return CanMergeOnRoute(Bounds, Layout, RouteInfoAtCell);
	};
	TArray<FIntRect> QuadCandidates = FindMergeCandidates(Layout, 2, 2, CanMerge);
	const TArray<FIntRect> PickedQuadBounds = PickMergePlaces(QuadCandidates, QuadRoomCount, SpreadCandidateCount, PlacedCenters, Stream);

	// 그다음 2칸. 4칸이 쓴 자리와 겹치는 후보는 뺀다
	TArray<FIntRect> DualCandidates = FindMergeCandidates(Layout, 2, 1, CanMerge);   // 가로 2칸
	DualCandidates.Append(FindMergeCandidates(Layout, 1, 2, CanMerge));             // 세로 2칸
	RemoveOverlappingCandidates(DualCandidates, PickedQuadBounds);
	const TArray<FIntRect> PickedDualBounds = PickMergePlaces(DualCandidates, DualRoomCount, SpreadCandidateCount, PlacedCenters, Stream);

	const int32 LeftoverQuadRoomCount = QuadRoomCount - PickedQuadBounds.Num();
	const int32 LeftoverDualRoomCount = DualRoomCount - PickedDualBounds.Num();

	TArray<FIntRect> PickedBounds = PickedQuadBounds;
	PickedBounds.Append(PickedDualBounds);

	// 4. 합친다. 실패하면 Layout을 그대로 두어야 하므로 복사본에서 한다
	FPMGLayout MergedLayout = Layout;
	MergeRooms(MergedLayout, PickedBounds);

	// 소속이 같은 칸끼리만 합치므로, 한 루트에만 있는 방은 합친 뒤에도 그 루트에만 있다 → 두 루트는 계속 다르다
	checkf(MergedLayout.PrimaryRoute != MergedLayout.SecondaryRoute, TEXT("MergeRouteBigRooms: 합친 뒤 두 루트가 같아짐"));

	// 5. 사용될 방 수: 루트 방 + Phase 6에서 루트 옆에 붙일 큰 방 (남은 큰 방, 추가방 예약). 큰 방 하나 = 방 하나
	const int32 RouteRoomCount = CountRoomsOnRoutes(MergedLayout);
	const int32 BesideRouteRoomCount = LeftoverQuadRoomCount + LeftoverDualRoomCount + ReservedRoomCount;
	if (RouteRoomCount + BesideRouteRoomCount > TotalRoomCount)
	{
		OutError = FString::Printf(TEXT("루트 방 수 초과: 큰 방을 합친 루트 1 + 루트 2의 합집합 %d개 + 루트 옆에 붙일 큰 방 %d개 (남은 4칸 %d, 남은 2칸 %d, 추가방 예약 %d) = %d개 > TotalRoomCount %d"),
			RouteRoomCount, BesideRouteRoomCount, LeftoverQuadRoomCount, LeftoverDualRoomCount, ReservedRoomCount, RouteRoomCount + BesideRouteRoomCount, TotalRoomCount);
		return false;
	}

	Layout = MoveTemp(MergedLayout);
	OutLeftoverQuadRoomCount = LeftoverQuadRoomCount;
	OutLeftoverDualRoomCount = LeftoverDualRoomCount;
	return true;
}

bool PMG::AttachBigRoomsBesideRoutes(FPMGLayout& Layout, int32 QuadRoomCount, int32 DualRoomCount, int32 SpreadCandidateCount,
	FRandomStream& Stream, FString& OutError)
{
	checkf(!Layout.Doors.IsEmpty(), TEXT("AttachBigRoomsBesideRoutes는 루트를 연결한 뒤(ConnectRoutes) 불러야 한다"));

	// 1. 이미 놓인 방: 입구/출구와 큰 방 (루트 위 큰 방). 루트 옆 큰 방도 이들에서 멀리 놓는다
	TArray<FVector2D> PlacedCenters;
	for (const FPMGRoom& Room : Layout.Rooms)
	{
		const bool bEntranceOrExit = Room.Id == Layout.EntranceRoomId || Room.Id == Layout.ExitRoomId;
		if (bEntranceOrExit || Room.Shape != EPMGRoomShape::Single)
		{
			PlacedCenters.Add(Room.GetCenter());
		}
	}

	// 2. 4칸 먼저, 그다음 2칸 (4칸이 쓴 자리와 겹치는 후보는 뺀다). 루트 위 큰 방(MergeRouteBigRooms)과 같은 순서와 방법
	auto CanAttach = [&Layout](const FIntRect& Bounds)
	{
		return CanAttachBesideRoute(Bounds, Layout);
	};
	TArray<FIntRect> QuadCandidates = FindMergeCandidates(Layout, 2, 2, CanAttach);
	const TArray<FIntRect> PickedQuadBounds = PickMergePlaces(QuadCandidates, QuadRoomCount, SpreadCandidateCount, PlacedCenters, Stream);

	TArray<FIntRect> DualCandidates = FindMergeCandidates(Layout, 2, 1, CanAttach);   // 가로 2칸
	DualCandidates.Append(FindMergeCandidates(Layout, 1, 2, CanAttach));             // 세로 2칸
	RemoveOverlappingCandidates(DualCandidates, PickedQuadBounds);
	const TArray<FIntRect> PickedDualBounds = PickMergePlaces(DualCandidates, DualRoomCount, SpreadCandidateCount, PlacedCenters, Stream);

	// 3. 자리가 모자라면 시도 실패 (아직 Layout을 바꾸지 않았다)
	if (PickedQuadBounds.Num() < QuadRoomCount || PickedDualBounds.Num() < DualRoomCount)
	{
		OutError = FString::Printf(TEXT("루트 옆 큰 방 자리 부족: 4칸 %d개 중 %d개, 2칸 %d개 중 %d개만 루트 방 옆에 자리를 찾음"),
			QuadRoomCount, PickedQuadBounds.Num(), DualRoomCount, PickedDualBounds.Num());
		return false;
	}

	// 4. 합친다. 고른 자리는 모두 미사용 칸이라 MergeRooms의 "연결된 방은 합치지 않음" 조건을 지킨다
	TArray<FIntRect> PickedBounds = PickedQuadBounds;
	PickedBounds.Append(PickedDualBounds);
	MergeRooms(Layout, PickedBounds);

	// 5. 붙인 큰 방마다 맞닿은 루트 방 중 하나를 무작위로 골라 문으로 잇는다 (고른 순서대로).
	//    합친 뒤 방 ID가 바뀌었으므로 큰 방은 왼쪽 위 셀로 찾는다. 큰 방의 이웃 목록 순서(오름차순)가 정해져 있어 결과가 시드로 정해진다
	for (const FIntRect& Bounds : PickedBounds)
	{
		const int32 BigRoomId = Layout.GetRoomIdAt(Bounds.Min);
		TArray<int32> RouteNeighborIds;
		for (const int32 NeighborId : Layout.Rooms[BigRoomId].Neighbors)
		{
			if (Layout.Rooms[NeighborId].IsRouteRoom())
			{
				RouteNeighborIds.Add(NeighborId);
			}
		}
		checkf(!RouteNeighborIds.IsEmpty(), TEXT("AttachBigRoomsBesideRoutes: 큰 방 %d에 맞닿은 루트 방이 없음"), BigRoomId);

		// 루트 방이 하나뿐이어도 난수를 쓴다 (ConnectRooms의 문 고르기와 같은 이유: 난수 사용 횟수가 후보 수에 따라 달라지지 않게)
		const int32 RouteRoomId = RouteNeighborIds[Stream.RandRange(0, RouteNeighborIds.Num() - 1)];
		ConnectRooms(Layout, RouteRoomId, BigRoomId, Stream);
		Layout.Rooms[BigRoomId].bExtra = true;
	}
	return true;
}
