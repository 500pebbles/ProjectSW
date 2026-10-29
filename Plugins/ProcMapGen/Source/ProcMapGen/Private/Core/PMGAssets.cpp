#include "Core/PMGAssets.h"
#include "Algo/RandomShuffle.h"

namespace
{
	/**
	 * 셔플 백 (3-5절): 에셋 인덱스 0 ~ VariantCount - 1을 섞어 앞에서부터 하나씩 꺼내고, 다 꺼내면 다시 섞는다.
	 * 그래서 한 바퀴(VariantCount개) 안에서는 같은 인덱스가 두 번 나오지 않는다
	 */
	struct FShuffleBag
	{
		TArray<int32> Indices;

		/** 다음에 꺼낼 Indices의 위치. Indices.Num()이면 다 꺼낸 상태라 다음 Take에서 섞는다 */
		int32 NextPosition = 0;

		explicit FShuffleBag(int32 VariantCount)
		{
			for (int32 Index = 0; Index < VariantCount; ++Index)
			{
				Indices.Add(Index);
			}
			NextPosition = Indices.Num();   // 첫 Take에서 섞는다
		}

		int32 Take(FRandomStream& Stream)
		{
			if (NextPosition >= Indices.Num())
			{
				Algo::RandomShuffle(Indices, Stream);
				NextPosition = 0;
			}
			const int32 VariantIndex = Indices[NextPosition];
			NextPosition++;
			return VariantIndex;
		}
	};

	const TCHAR* GetRoomSizeText(EPMGRoomSize Size)
	{
		switch (Size)
		{
		case EPMGRoomSize::Dual:
			return TEXT("2칸");
		case EPMGRoomSize::Quad:
			return TEXT("4칸");
		default:
			return TEXT("1칸");
		}
	}

	/** FPMGRoomTypeAssets에서 Size 크기 리스트의 필드 이름 (메시지에 표시) */
	const TCHAR* GetRoomListName(EPMGRoomSize Size)
	{
		switch (Size)
		{
		case EPMGRoomSize::Dual:
			return TEXT("DualRooms");
		case EPMGRoomSize::Quad:
			return TEXT("QuadRooms");
		default:
			return TEXT("SingleRooms");
		}
	}

	/** 에셋 배정 그룹의 방 타입 순서: Entrance, Exit, Standard, 커스텀 타입 (AssetLists.CustomTypes 순서) */
	TArray<FName> GetAssetRoomTypeOrder(const FPMGAssetLists& AssetLists)
	{
		TArray<FName> RoomTypes = { PMG::RoomTypeEntrance(), PMG::RoomTypeExit(), PMG::RoomTypeStandard() };
		for (const FPMGCustomRoomType& CustomType : AssetLists.CustomTypes)
		{
			RoomTypes.Add(CustomType.TypeName);
		}
		return RoomTypes;
	}

	/**
	 * 스폰되는 방인지 = 방 에셋이 배정된 사용된 방. 리스트가 비어 있는 사용된 방은 VariantIndex가 INDEX_NONE이다.
	 * 미사용 방은 채움 방 에셋(PMG::AssignFillerAssets)이 있어도 스폰되는 방으로 보지 않는다 (3-4절 표: 채움 방은 "스폰되지 않는 쪽")
	 */
	bool IsRoomSpawned(const FPMGLayout& Layout, int32 RoomId)
	{
		const FPMGRoom& Room = Layout.Rooms[RoomId];
		return Room.bConnected && Room.VariantIndex != INDEX_NONE;
	}

	/** 회전을 허용하는 방 에셋의 회전 (3-5절): 1칸/4칸 방은 0~3, 2칸 방은 90°를 더 돌리면 가로·세로가 바뀌어 영역에 맞지 않으므로 0° 또는 180° */
	uint8 ChooseRoomRotation(EPMGRoomSize Size, FRandomStream& Stream)
	{
		const int32 Rotation = Size == EPMGRoomSize::Dual ? Stream.RandRange(0, 1) * 2 : Stream.RandRange(0, 3);
		return static_cast<uint8>(Rotation);
	}

	/** 맵 안의 맞닿은 두 셀 사이 경계를 찾는 키: 두 셀 인덱스(Y * Cols + X)를 (작은 쪽, 큰 쪽) 순서로. 두 셀의 순서와 무관하게 같은 키가 된다 */
	FIntPoint MakeDoorEdgeKey(const FPMGLayout& Layout, const FIntPoint& CellA, const FIntPoint& CellB)
	{
		// 2D 좌표 (X, Y) → 1D 인덱스: 왼쪽 위부터 한 줄씩 센 번호. 맵 안의 셀마다 서로 다른 번호가 된다
		const int32 CellIndexA = CellA.Y * Layout.Cols + CellA.X;
		const int32 CellIndexB = CellB.Y * Layout.Cols + CellB.X;
		return FIntPoint(FMath::Min(CellIndexA, CellIndexB), FMath::Max(CellIndexA, CellIndexB));
	}

	/**
	 * 커넥터 중 종류가 Type이고 문 리스트가 ListRoomType인 것(커넥터 순서)에 AssetSet의 에셋을 셔플 백으로 배정한다.
	 * 리스트가 비어 있으면 그대로 둔다 (INDEX_NONE). ListText는 메시지에 표시할 리스트 이름
	 */
	void AssignConnectorGroup(TArray<FPMGConnector>& Connectors, EPMGConnectorType Type, FName ListRoomType, const FPMGAssetSet& AssetSet,
		const FString& ListText, FRandomStream& Stream, TArray<FString>& OutInfos)
	{
		TArray<int32> GroupConnectorIndices;
		for (int32 ConnectorIndex = 0; ConnectorIndex < Connectors.Num(); ++ConnectorIndex)
		{
			if (Connectors[ConnectorIndex].Type == Type && Connectors[ConnectorIndex].DoorListRoomType == ListRoomType)
			{
				GroupConnectorIndices.Add(ConnectorIndex);
			}
		}
		if (GroupConnectorIndices.IsEmpty() || AssetSet.IsEmpty())
		{
			return;
		}

		if (AssetSet.Variants.Num() < GroupConnectorIndices.Num())
		{
			OutInfos.Add(FString::Printf(TEXT("%s: %s %d개 > 에셋 %d개 → 같은 에셋이 반복됨"), *ListText,
				Type == EPMGConnectorType::Door ? TEXT("문") : TEXT("벽"), GroupConnectorIndices.Num(), AssetSet.Variants.Num()));
		}
		FShuffleBag ShuffleBag(AssetSet.Variants.Num());
		for (const int32 ConnectorIndex : GroupConnectorIndices)
		{
			Connectors[ConnectorIndex].VariantIndex = ShuffleBag.Take(Stream);
		}
	}
}

const FPMGRoomTypeAssets* FPMGAssetLists::FindRoomTypeAssets(FName RoomType) const
{
	if (RoomType == PMG::RoomTypeEntrance())
	{
		return &EntranceRooms;
	}
	if (RoomType == PMG::RoomTypeExit())
	{
		return &ExitRooms;
	}
	if (RoomType == PMG::RoomTypeStandard())
	{
		return &StandardRooms;
	}
	for (const FPMGCustomRoomType& CustomType : CustomTypes)
	{
		if (CustomType.TypeName == RoomType)
		{
			return &CustomType.Assets;
		}
	}
	return nullptr;
}

void PMG::AssignRoomAssets(FPMGLayout& Layout, const FPMGAssetLists& AssetLists, FRandomStream& Stream,
	TArray<FString>& OutWarnings, TArray<FString>& OutInfos)
{
	const EPMGRoomSize RoomSizes[] = { EPMGRoomSize::Single, EPMGRoomSize::Dual, EPMGRoomSize::Quad };

	for (const FName RoomType : GetAssetRoomTypeOrder(AssetLists))
	{
		const FPMGRoomTypeAssets* TypeAssets = AssetLists.FindRoomTypeAssets(RoomType);   // 순서 목록의 타입이라 항상 있다
		for (const EPMGRoomSize Size : RoomSizes)
		{
			// 1. 그룹 = 이 타입, 이 크기의 사용된 방 (방 ID 오름차순)
			TArray<int32> GroupRoomIds;
			for (const FPMGRoom& Room : Layout.Rooms)
			{
				if (Room.bConnected && Room.RoomType == RoomType && PMG::GetRoomSize(Room.Shape) == Size)
				{
					GroupRoomIds.Add(Room.Id);
				}
			}
			if (GroupRoomIds.IsEmpty())
			{
				continue;
			}

			// 2. 리스트가 비어 있으면 스폰 생략 (1-2절 ① 무시 규칙). 방 구조와 타입은 그대로 두고 VariantIndex만 INDEX_NONE
			const FPMGAssetSet& RoomAssets = TypeAssets->GetRoomAssets(Size);
			if (RoomAssets.IsEmpty())
			{
				OutWarnings.Add(FString::Printf(TEXT("방 에셋: 타입 %s의 %s 방 %d개 → 리스트(%s)가 비어 있어 스폰 생략"),
					*RoomType.ToString(), GetRoomSizeText(Size), GroupRoomIds.Num(), GetRoomListName(Size)));
				continue;
			}
			if (RoomAssets.Variants.Num() < GroupRoomIds.Num())
			{
				OutInfos.Add(FString::Printf(TEXT("방 에셋: 타입 %s의 %s 방 %d개 > 에셋 %d개(%s) → 같은 에셋이 반복됨"),
					*RoomType.ToString(), GetRoomSizeText(Size), GroupRoomIds.Num(), RoomAssets.Variants.Num(), GetRoomListName(Size)));
			}

			// 3. 셔플 백으로 에셋, 회전을 허용하는 에셋이면 회전 (방마다 에셋 → 회전 순서로 난수를 쓴다)
			FShuffleBag ShuffleBag(RoomAssets.Variants.Num());
			for (const int32 RoomId : GroupRoomIds)
			{
				FPMGRoom& Room = Layout.Rooms[RoomId];
				Room.VariantIndex = ShuffleBag.Take(Stream);
				Room.Rotation = 0;
				if (RoomAssets.Variants[Room.VariantIndex].bAllowRotation)
				{
					Room.Rotation = ChooseRoomRotation(Size, Stream);
				}
			}
		}
	}
}

void PMG::BuildConnectors(FPMGLayout& Layout)
{
	Layout.Connectors.Reset();

	// 문 자리 찾기용: 경계 키 → 문 인덱스. 찾기에만 쓰고 순회하지 않으므로 TMap의 순서는 결과에 영향이 없다
	TMap<FIntPoint, int32> DoorIndexByEdge;
	for (int32 DoorIndex = 0; DoorIndex < Layout.Doors.Num(); ++DoorIndex)
	{
		const FPMGDoor& Door = Layout.Doors[DoorIndex];
		DoorIndexByEdge.Add(MakeDoorEdgeKey(Layout, Door.CellA, Door.CellB), DoorIndex);
	}

	// PMG::BuildNeighbors와 같은 방향 순서
	const FIntPoint Directions[] = { FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(-1, 0), FIntPoint(0, -1) };
	for (int32 Y = 0; Y < Layout.Rows; ++Y)
	{
		for (int32 X = 0; X < Layout.Cols; ++X)
		{
			const FIntPoint Cell(X, Y);
			const int32 RoomId = Layout.GetRoomIdAt(Cell);
			for (const FIntPoint& Direction : Directions)
			{
				const FIntPoint OtherCell = Cell + Direction;
				const bool bOtherCellInsideMap = Layout.IsInsideMap(OtherCell);
				int32 OtherRoomId = INDEX_NONE;
				if (bOtherCellInsideMap)
				{
					// 안쪽 경계는 +X, +Y 쪽에서만 센다 (한 경계를 양쪽 셀에서 두 번 세지 않게). 같은 방 안의 셀 사이는 경계가 아니다
					const bool bForwardDirection = Direction.X > 0 || Direction.Y > 0;
					OtherRoomId = Layout.GetRoomIdAt(OtherCell);
					if (!bForwardDirection || OtherRoomId == RoomId)
					{
						continue;
					}
				}

				const bool bRoomSpawned = IsRoomSpawned(Layout, RoomId);
				const bool bOtherRoomSpawned = OtherRoomId != INDEX_NONE && IsRoomSpawned(Layout, OtherRoomId);
				if (!bRoomSpawned && !bOtherRoomSpawned)
				{
					continue;   // 스폰되는 방이 접하지 않은 경계는 채울 필요가 없다
				}

				FPMGConnector Connector;
				Connector.CellA = Cell;
				Connector.CellB = OtherCell;
				Connector.Type = EPMGConnectorType::Wall;
				if (bRoomSpawned && bOtherRoomSpawned)
				{
					// 두 방이 모두 스폰되고 이 경계가 문 자리면 문. 방향은 문의 RoomA → RoomB를 따른다
					if (const int32* DoorIndex = DoorIndexByEdge.Find(MakeDoorEdgeKey(Layout, Cell, OtherCell)))
					{
						const FPMGDoor& Door = Layout.Doors[*DoorIndex];
						Connector.Type = EPMGConnectorType::Door;
						Connector.CellA = Door.CellA;
						Connector.CellB = Door.CellB;
					}
				}
				Layout.Connectors.Add(Connector);
			}
		}
	}
}

void PMG::AssignConnectorAssets(FPMGLayout& Layout, const FPMGAssetLists& AssetLists, FRandomStream& Stream, TArray<FString>& OutInfos)
{
	// 1. 문마다 쓸 리스트: 들어가는 쪽(RoomB) 방 타입의 Doors → 반대쪽(RoomA) 방 타입의 Doors → 전역 Doors (NAME_None)
	for (FPMGConnector& Connector : Layout.Connectors)
	{
		Connector.DoorListRoomType = NAME_None;
		Connector.VariantIndex = INDEX_NONE;
		if (Connector.Type != EPMGConnectorType::Door)
		{
			continue;
		}
		const FName RoomTypeA = Layout.Rooms[Layout.GetRoomIdAt(Connector.CellA)].RoomType;
		const FName RoomTypeB = Layout.Rooms[Layout.GetRoomIdAt(Connector.CellB)].RoomType;
		const FPMGRoomTypeAssets* AssetsA = AssetLists.FindRoomTypeAssets(RoomTypeA);
		const FPMGRoomTypeAssets* AssetsB = AssetLists.FindRoomTypeAssets(RoomTypeB);
		if (AssetsB && !AssetsB->Doors.IsEmpty())
		{
			Connector.DoorListRoomType = RoomTypeB;
		}
		else if (AssetsA && !AssetsA->Doors.IsEmpty())
		{
			Connector.DoorListRoomType = RoomTypeA;
		}
	}

	// 2. 리스트마다 셔플 백: 방 타입의 Doors → 전역 Doors → Walls. 회전은 정하지 않는다
	for (const FName RoomType : GetAssetRoomTypeOrder(AssetLists))
	{
		const FPMGRoomTypeAssets* TypeAssets = AssetLists.FindRoomTypeAssets(RoomType);
		AssignConnectorGroup(Layout.Connectors, EPMGConnectorType::Door, RoomType, TypeAssets->Doors,
			FString::Printf(TEXT("문 에셋: 타입 %s의 Doors"), *RoomType.ToString()), Stream, OutInfos);
	}
	AssignConnectorGroup(Layout.Connectors, EPMGConnectorType::Door, NAME_None, AssetLists.Doors, TEXT("문 에셋: 전역 Doors"), Stream, OutInfos);
	AssignConnectorGroup(Layout.Connectors, EPMGConnectorType::Wall, NAME_None, AssetLists.Walls, TEXT("벽 에셋: Walls"), Stream, OutInfos);
}

void PMG::AssignFillerAssets(FPMGLayout& Layout, const FPMGAssetLists& AssetLists, FRandomStream& Stream, TArray<FString>& OutInfos)
{
	const FPMGAssetSet& FillerRooms = AssetLists.FillerRooms;
	if (FillerRooms.IsEmpty())
	{
		return;   // 채움 방을 스폰하지 않는 설정 (난수도 쓰지 않는다)
	}

	// 그룹 = 미사용 방 전체 (방 ID 오름차순). 큰 방은 모두 사용되므로 보통 1칸 방뿐이지만, 회전은 방 크기대로 정한다
	TArray<int32> UnusedRoomIds;
	for (const FPMGRoom& Room : Layout.Rooms)
	{
		if (!Room.bConnected)
		{
			UnusedRoomIds.Add(Room.Id);
		}
	}
	if (UnusedRoomIds.IsEmpty())
	{
		return;
	}
	if (FillerRooms.Variants.Num() < UnusedRoomIds.Num())
	{
		OutInfos.Add(FString::Printf(TEXT("채움 방 에셋: 미사용 방 %d개 > 에셋 %d개(FillerRooms) → 같은 에셋이 반복됨"),
			UnusedRoomIds.Num(), FillerRooms.Variants.Num()));
	}

	// 방 에셋과 같은 셔플 백과 회전 (방마다 에셋 → 회전 순서로 난수를 쓴다)
	FShuffleBag ShuffleBag(FillerRooms.Variants.Num());
	for (const int32 RoomId : UnusedRoomIds)
	{
		FPMGRoom& Room = Layout.Rooms[RoomId];
		Room.VariantIndex = ShuffleBag.Take(Stream);
		Room.Rotation = 0;
		if (FillerRooms.Variants[Room.VariantIndex].bAllowRotation)
		{
			Room.Rotation = ChooseRoomRotation(PMG::GetRoomSize(Room.Shape), Stream);
		}
	}
}
