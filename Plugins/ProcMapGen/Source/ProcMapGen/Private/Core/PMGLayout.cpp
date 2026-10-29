#include "Core/PMGLayout.h"
#include "Core/PMGNeighbors.h"

namespace
{
	// ============================================================
	// 방 만들기
	// ============================================================

	EPMGRoomShape GetRoomShape(const FIntRect& Bounds)
	{
		const int32 Width = Bounds.Width();
		const int32 Height = Bounds.Height();

		if (Width == 2 && Height == 2)
		{
			return EPMGRoomShape::Quad;
		}
		if (Width == 2 && Height == 1)
		{
			return EPMGRoomShape::DualHorizontal;
		}
		if (Width == 1 && Height == 2)
		{
			return EPMGRoomShape::DualVertical;
		}
		return EPMGRoomShape::Single;
	}

	/** 방 하나를 목록에 추가하고, 방의 셀마다 CellToRoom에 방 ID를 적는다 */
	void AddRoom(FPMGLayout& Layout, const FIntRect& Bounds)
	{
		FPMGRoom Room;
		Room.Id = Layout.Rooms.Num();
		Room.Bounds = Bounds;
		Room.Shape = GetRoomShape(Bounds);
		Layout.Rooms.Add(Room);

		for (int32 Y = Bounds.Min.Y; Y < Bounds.Max.Y; ++Y)
		{
			for (int32 X = Bounds.Min.X; X < Bounds.Max.X; ++X)
			{
				Layout.CellToRoom[Y * Layout.Cols + X] = Room.Id;
			}
		}
	}

	/**
	 * 큰 방 목록으로 방 목록과 CellToRoom을 새로 만든다. 큰 방이 아닌 셀은 모두 1칸 방이다.
	 * 셀을 위쪽 줄부터, 왼쪽부터 훑으며 아직 방이 없는 셀을 만나면 그 셀을 왼쪽 위로 하는 방을 만든다.
	 * 그래서 방 ID는 큰 방을 만든 순서와 상관없이 항상 "방의 왼쪽 위 셀" 순서로 붙는다.
	 */
	void BuildRooms(FPMGLayout& Layout, const TArray<FIntRect>& BigRoomBounds)
	{
		const int32 CellCount = Layout.Rows * Layout.Cols;
		Layout.Rooms.Reset();
		Layout.CellToRoom.Init(INDEX_NONE, CellCount);

		// 셀마다 그 셀을 덮는 큰 방 (BigRoomBounds 인덱스). 큰 방의 셀이 아니면 INDEX_NONE
		TArray<int32> BigRoomIndexAtCell;
		BigRoomIndexAtCell.Init(INDEX_NONE, CellCount);
		for (int32 BigRoomIndex = 0; BigRoomIndex < BigRoomBounds.Num(); ++BigRoomIndex)
		{
			const FIntRect& Bounds = BigRoomBounds[BigRoomIndex];
			for (int32 Y = Bounds.Min.Y; Y < Bounds.Max.Y; ++Y)
			{
				for (int32 X = Bounds.Min.X; X < Bounds.Max.X; ++X)
				{
					BigRoomIndexAtCell[Y * Layout.Cols + X] = BigRoomIndex;
				}
			}
		}

		for (int32 Y = 0; Y < Layout.Rows; ++Y)
		{
			for (int32 X = 0; X < Layout.Cols; ++X)
			{
				const int32 CellIndex = Y * Layout.Cols + X;
				if (Layout.CellToRoom[CellIndex] != INDEX_NONE)
				{
					continue;   // 앞에서 만든 큰 방의 셀
				}

				const int32 BigRoomIndex = BigRoomIndexAtCell[CellIndex];
				if (BigRoomIndex != INDEX_NONE)
				{
					AddRoom(Layout, BigRoomBounds[BigRoomIndex]);   // 이 셀이 큰 방의 왼쪽 위 셀
				}
				else
				{
					AddRoom(Layout, FIntRect(X, Y, X + 1, Y + 1));   // 1칸 방
				}
			}
		}
	}

	/** 루트의 방마다 그 방의 왼쪽 위 셀. 방 ID가 바뀌어도 셀은 그대로이므로, 방을 다시 만든 뒤 이 셀로 새 방 ID를 찾는다 */
	TArray<FIntPoint> GetRouteCells(const FPMGLayout& Layout, const TArray<int32>& Route)
	{
		TArray<FIntPoint> Cells;
		for (const int32 RoomId : Route)
		{
			Cells.Add(Layout.Rooms[RoomId].Bounds.Min);
		}
		return Cells;
	}

	/** 셀 목록을 지금의 방 ID 목록으로 바꾼다. 합쳐져 같은 방이 된 셀이 연달아 나오면 한 번만 넣는다 */
	TArray<int32> GetRouteFromCells(const FPMGLayout& Layout, const TArray<FIntPoint>& Cells)
	{
		TArray<int32> Route;
		for (const FIntPoint& Cell : Cells)
		{
			const int32 RoomId = Layout.GetRoomIdAt(Cell);
			if (Route.IsEmpty() || Route.Last() != RoomId)
			{
				Route.Add(RoomId);
			}
		}
		return Route;
	}

	// ============================================================
	// 디버그 출력
	// ============================================================

	TCHAR GetShapeChar(EPMGRoomShape Shape)
	{
		switch (Shape)
		{
		case EPMGRoomShape::Quad:
			return TEXT('4');
		case EPMGRoomShape::DualHorizontal:
		case EPMGRoomShape::DualVertical:
			return TEXT('2');
		default:
			return TEXT('1');
		}
	}
}

TArray<FString> FPMGLayout::ToDebugLines() const
{
	// 셀 문자: 입구방 E, 출구방 X, 그 밖은 방 칸 수
	return DrawRoomMap([this](int32 RoomId) -> TCHAR
	{
		if (RoomId == EntranceRoomId)
		{
			return TEXT('E');
		}
		if (RoomId == ExitRoomId)
		{
			return TEXT('X');
		}
		return GetShapeChar(Rooms[RoomId].Shape);
	});
}

TArray<FString> FPMGLayout::ToRouteDebugLines() const
{
	// 방마다 루트 1 / 루트 2에 들어 있는지
	TArray<bool> bIsOnPrimaryRoute;
	TArray<bool> bIsOnSecondaryRoute;
	bIsOnPrimaryRoute.Init(false, Rooms.Num());
	bIsOnSecondaryRoute.Init(false, Rooms.Num());
	for (const int32 RoomId : PrimaryRoute)
	{
		bIsOnPrimaryRoute[RoomId] = true;
	}
	for (const int32 RoomId : SecondaryRoute)
	{
		bIsOnSecondaryRoute[RoomId] = true;
	}

	// 셀 문자: 입구방 E, 출구방 X, 두 루트 공통 B, 루트 1만 1, 루트 2만 2, 추가방 *, 그 밖(미사용) .
	return DrawRoomMap([&](int32 RoomId) -> TCHAR
	{
		if (RoomId == EntranceRoomId)
		{
			return TEXT('E');
		}
		if (RoomId == ExitRoomId)
		{
			return TEXT('X');
		}
		if (Rooms[RoomId].bExtra)
		{
			return TEXT('*');
		}
		if (bIsOnPrimaryRoute[RoomId] && bIsOnSecondaryRoute[RoomId])
		{
			return TEXT('B');
		}
		if (bIsOnPrimaryRoute[RoomId])
		{
			return TEXT('1');
		}
		if (bIsOnSecondaryRoute[RoomId])
		{
			return TEXT('2');
		}
		return TEXT('.');
	});
}

TArray<FString> FPMGLayout::DrawRoomMap(TFunctionRef<TCHAR(int32 RoomId)> GetRoomChar) const
{
	// 맵 밖이면 INDEX_NONE
	auto GetRoomAt = [this](int32 X, int32 Y) -> int32
	{
		if (!IsInsideMap(FIntPoint(X, Y)))
		{
			return INDEX_NONE;
		}
		return CellToRoom[Y * Cols + X];
	};

	// 셀마다 오른쪽 경계와 아래쪽 경계에 문이 있는지. 문의 두 셀 중 왼쪽(가로로 맞닿음) 또는 위쪽(세로로 맞닿음) 셀에 기록한다
	TArray<bool> bDoorRightOfCell;
	TArray<bool> bDoorBelowCell;
	bDoorRightOfCell.Init(false, Rows * Cols);
	bDoorBelowCell.Init(false, Rows * Cols);
	for (const FPMGDoor& Door : Doors)
	{
		const int32 LeftOrTopX = FMath::Min(Door.CellA.X, Door.CellB.X);
		const int32 LeftOrTopY = FMath::Min(Door.CellA.Y, Door.CellB.Y);
		if (Door.CellA.Y == Door.CellB.Y)
		{
			bDoorRightOfCell[LeftOrTopY * Cols + LeftOrTopX] = true;
		}
		else
		{
			bDoorBelowCell[LeftOrTopY * Cols + LeftOrTopX] = true;
		}
	}

	TArray<FString> Lines;

	// 맨 위 테두리: "+-+-+-+"
	FString TopLine = TEXT("+");
	for (int32 X = 0; X < Cols; ++X)
	{
		TopLine += TEXT("-+");
	}
	Lines.Add(TopLine);

	for (int32 Y = 0; Y < Rows; ++Y)
	{
		// 셀 줄: "|1|4 4#1|". 오른쪽 셀과 같은 방이면 '|'를 비우고, 문이 있으면 '#'
		FString CellLine = TEXT("|");
		for (int32 X = 0; X < Cols; ++X)
		{
			const int32 RoomId = GetRoomAt(X, Y);

			// 셀 문자는 호출한 쪽이 정한다 (방 크기 / 루트 등)
			const TCHAR CellChar = Rooms.IsValidIndex(RoomId) ? GetRoomChar(RoomId) : TEXT('?');
			CellLine.AppendChar(CellChar);

			const bool bSameRoomAsRight = GetRoomAt(X + 1, Y) == RoomId;
			if (bSameRoomAsRight)
			{
				CellLine += TEXT(" ");
			}
			else if (bDoorRightOfCell[Y * Cols + X])
			{
				CellLine += TEXT("#");
			}
			else
			{
				CellLine += TEXT("|");
			}
		}
		Lines.Add(CellLine);

		// 아래 경계 줄: "+-+ +#+". 아래 셀과 같은 방이면 '-'를 비우고, 문이 있으면 '#', 꼭짓점 주변 4칸이 모두 같은 방이면 '+'도 비운다
		FString BorderLine = TEXT("+");
		for (int32 X = 0; X < Cols; ++X)
		{
			const int32 RoomId = GetRoomAt(X, Y);
			const bool bSameRoomAsBelow = GetRoomAt(X, Y + 1) == RoomId;
			if (bSameRoomAsBelow)
			{
				BorderLine += TEXT(" ");
			}
			else if (bDoorBelowCell[Y * Cols + X])
			{
				BorderLine += TEXT("#");
			}
			else
			{
				BorderLine += TEXT("-");
			}

			const bool bCornerInsideRoom = bSameRoomAsBelow
				&& GetRoomAt(X + 1, Y) == RoomId
				&& GetRoomAt(X + 1, Y + 1) == RoomId;
			BorderLine += bCornerInsideRoom ? TEXT(" ") : TEXT("+");
		}
		Lines.Add(BorderLine);
	}

	return Lines;
}

int32 PMG::GetMaxQuadRoomCount(int32 Rows, int32 Cols)
{
	return (Rows / 2) * (Cols / 2);
}

int32 PMG::GetMaxDualRoomCount(int32 Rows, int32 Cols, int32 QuadRoomCount)
{
	if (Rows * Cols < 2)
	{
		return 0;   // 1x1 맵에는 2칸 방이 들어가지 않는다
	}
	return (Rows * Cols - 4 * QuadRoomCount) / 2;
}

bool PMG::CanFitBigRooms(int32 Rows, int32 Cols, int32 QuadRoomCount, int32 DualRoomCount)
{
	if (QuadRoomCount > GetMaxQuadRoomCount(Rows, Cols))
	{
		return false;
	}
	return DualRoomCount <= GetMaxDualRoomCount(Rows, Cols, QuadRoomCount);
}

void PMG::BuildLayout(int32 Rows, int32 Cols, FPMGLayout& OutLayout)
{
	checkf(Rows >= 1 && Cols >= 1, TEXT("잘못된 맵 크기 (Rows=%d, Cols=%d)"), Rows, Cols);

	OutLayout = FPMGLayout();
	OutLayout.Rows = Rows;
	OutLayout.Cols = Cols;

	// 큰 방이 없으므로 모든 셀이 1칸 방이 된다
	BuildRooms(OutLayout, TArray<FIntRect>());
}

void PMG::MergeRooms(FPMGLayout& Layout, const TArray<FIntRect>& NewBigRoomBounds)
{
	// 에셋과 커넥터는 다시 맞추지 않는다 (에셋 배정은 합치기가 모두 끝난 뒤에 한다)
	checkf(Layout.Connectors.IsEmpty(), TEXT("MergeRooms: 에셋 배정(커넥터)이 끝난 Layout은 합칠 수 없음"));

	// 1. 방 ID가 바뀌기 전에, 다시 맞춰야 할 것들을 셀로 기억한다
	const bool bHasEntrance = Layout.Rooms.IsValidIndex(Layout.EntranceRoomId);
	const bool bHasExit = Layout.Rooms.IsValidIndex(Layout.ExitRoomId);
	const FIntPoint EntranceCell = bHasEntrance ? Layout.Rooms[Layout.EntranceRoomId].Bounds.Min : FIntPoint::ZeroValue;
	const FIntPoint ExitCell = bHasExit ? Layout.Rooms[Layout.ExitRoomId].Bounds.Min : FIntPoint::ZeroValue;
	const TArray<FIntPoint> PrimaryRouteCells = GetRouteCells(Layout, Layout.PrimaryRoute);
	const TArray<FIntPoint> SecondaryRouteCells = GetRouteCells(Layout, Layout.SecondaryRoute);
	TArray<FIntPoint> ConnectedRoomCells;
	TArray<FIntPoint> ExtraRoomCells;
	for (const FPMGRoom& Room : Layout.Rooms)
	{
		if (Room.bConnected)
		{
			ConnectedRoomCells.Add(Room.Bounds.Min);
		}
		if (Room.bExtra)
		{
			ExtraRoomCells.Add(Room.Bounds.Min);
		}
	}
	// 문(Doors)은 처음부터 셀(CellA, CellB)을 가지고 있으므로 따로 기억하지 않는다

	// 2. 큰 방 목록 = 이미 있던 큰 방 + 새 큰 방
	TArray<FIntRect> BigRoomBounds;
	for (const FPMGRoom& Room : Layout.Rooms)
	{
		if (Room.Shape != EPMGRoomShape::Single)
		{
			BigRoomBounds.Add(Room.Bounds);
		}
	}
	TArray<bool> bIsNewBigRoomCell;
	bIsNewBigRoomCell.Init(false, Layout.Rows * Layout.Cols);
	for (const FIntRect& Bounds : NewBigRoomBounds)
	{
		const bool bValidShape = GetRoomShape(Bounds) != EPMGRoomShape::Single;
		const bool bInsideMap = Layout.IsInsideMap(Bounds);
		checkf(bValidShape && bInsideMap, TEXT("MergeRooms: 큰 방 모양이 아니거나 맵 밖 ((%d,%d)-(%d,%d))"), Bounds.Min.X, Bounds.Min.Y, Bounds.Max.X, Bounds.Max.Y);

		for (int32 Y = Bounds.Min.Y; Y < Bounds.Max.Y; ++Y)
		{
			for (int32 X = Bounds.Min.X; X < Bounds.Max.X; ++X)
			{
				const int32 CellIndex = Y * Layout.Cols + X;
				const FPMGRoom& CellRoom = Layout.Rooms[Layout.CellToRoom[CellIndex]];
				const bool bSingleRoomCell = CellRoom.Shape == EPMGRoomShape::Single;
				checkf(bSingleRoomCell && !bIsNewBigRoomCell[CellIndex], TEXT("MergeRooms: 셀 (%d, %d)이 1칸 방이 아니거나 새 큰 방끼리 겹침"), X, Y);
				checkf(!CellRoom.bConnected, TEXT("MergeRooms: 셀 (%d, %d)은 이미 연결된 방 (문이 큰 방 안쪽에 남을 수 있음)"), X, Y);
				bIsNewBigRoomCell[CellIndex] = true;
			}
		}
		BigRoomBounds.Add(Bounds);
	}

	// 3. 방 목록과 CellToRoom을 새로 만든다 (RoomType은 모두 기본값 Standard, bConnected와 bExtra는 모두 false가 된다)
	BuildRooms(Layout, BigRoomBounds);

	// 4. 기억해 둔 셀로 입구/출구, 루트, 연결 여부, 추가방 여부, 문을 다시 맞춘다
	if (bHasEntrance)
	{
		Layout.EntranceRoomId = Layout.GetRoomIdAt(EntranceCell);
		Layout.Rooms[Layout.EntranceRoomId].RoomType = RoomTypeEntrance();
	}
	if (bHasExit)
	{
		Layout.ExitRoomId = Layout.GetRoomIdAt(ExitCell);
		Layout.Rooms[Layout.ExitRoomId].RoomType = RoomTypeExit();
	}
	Layout.PrimaryRoute = GetRouteFromCells(Layout, PrimaryRouteCells);
	Layout.SecondaryRoute = GetRouteFromCells(Layout, SecondaryRouteCells);
	for (const FIntPoint& Cell : ConnectedRoomCells)
	{
		Layout.Rooms[Layout.GetRoomIdAt(Cell)].bConnected = true;
	}
	for (const FIntPoint& Cell : ExtraRoomCells)
	{
		Layout.Rooms[Layout.GetRoomIdAt(Cell)].bExtra = true;
	}
	for (FPMGDoor& Door : Layout.Doors)
	{
		Door.RoomA = Layout.GetRoomIdAt(Door.CellA);
		Door.RoomB = Layout.GetRoomIdAt(Door.CellB);
	}

	// 5. 이웃도 방 ID 기준이므로 다시 만든다
	BuildNeighbors(Layout);
}

bool PMG::VerifyLayout(const FPMGLayout& Layout, int32 QuadRoomCount, int32 DualRoomCount, FString& OutError)
{
	const int32 CellCount = Layout.Rows * Layout.Cols;
	if (Layout.CellToRoom.Num() != CellCount)
	{
		OutError = FString::Printf(TEXT("CellToRoom 크기 불일치 (%d != %d)"), Layout.CellToRoom.Num(), CellCount);
		return false;
	}

	int32 FoundQuadRooms = 0;
	int32 FoundDualRooms = 0;
	int32 FoundSingleRooms = 0;
	int32 TotalArea = 0;

	for (int32 RoomId = 0; RoomId < Layout.Rooms.Num(); ++RoomId)
	{
		const FPMGRoom& Room = Layout.Rooms[RoomId];
		const int32 Width = Room.Bounds.Width();
		const int32 Height = Room.Bounds.Height();

		// 방 하나: ID, 크기(1~2칸), 모양이 올바른지
		const bool bValidSize = Width >= 1 && Width <= 2 && Height >= 1 && Height <= 2;
		if (Room.Id != RoomId || !bValidSize || Room.Shape != GetRoomShape(Room.Bounds))
		{
			OutError = FString::Printf(TEXT("방 %d의 Id/크기/모양이 잘못됨 (%dx%d)"), RoomId, Width, Height);
			return false;
		}

		if (Room.Shape == EPMGRoomShape::Quad)
		{
			FoundQuadRooms++;
		}
		else if (Room.Shape == EPMGRoomShape::Single)
		{
			FoundSingleRooms++;
		}
		else
		{
			FoundDualRooms++;
		}
		TotalArea += Width * Height;

		// 방의 모든 셀이 CellToRoom에서 이 방을 가리키는지 (방끼리 겹치면 여기서 걸린다)
		for (int32 Y = Room.Bounds.Min.Y; Y < Room.Bounds.Max.Y; ++Y)
		{
			for (int32 X = Room.Bounds.Min.X; X < Room.Bounds.Max.X; ++X)
			{
				if (!Layout.IsInsideMap(FIntPoint(X, Y)) || Layout.CellToRoom[Y * Layout.Cols + X] != RoomId)
				{
					OutError = FString::Printf(TEXT("방 %d의 셀 (%d, %d)이 CellToRoom과 불일치"), RoomId, X, Y);
					return false;
				}
			}
		}
	}

	// 겹침이 없고 면적 합이 셀 수와 같으면 빈 셀도 없다
	if (TotalArea != CellCount)
	{
		OutError = FString::Printf(TEXT("방 면적 합(%d)이 셀 수(%d)와 다름"), TotalArea, CellCount);
		return false;
	}

	const int32 ExpectedSingleRooms = CellCount - 4 * QuadRoomCount - 2 * DualRoomCount;
	if (FoundQuadRooms != QuadRoomCount || FoundDualRooms != DualRoomCount || FoundSingleRooms != ExpectedSingleRooms)
	{
		OutError = FString::Printf(TEXT("크기별 방 개수 불일치 (4칸 %d/%d, 2칸 %d/%d, 1칸 %d/%d)"),
			FoundQuadRooms, QuadRoomCount, FoundDualRooms, DualRoomCount, FoundSingleRooms, ExpectedSingleRooms);
		return false;
	}
	return true;
}
