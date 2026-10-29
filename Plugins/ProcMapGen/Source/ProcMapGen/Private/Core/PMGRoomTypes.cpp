#include "Core/PMGRoomTypes.h"
#include "Algo/RandomShuffle.h"

namespace
{
	/** 커스텀 타입 하나의 처리 순서를 정하는 값 */
	struct FCustomTypeOrder
	{
		/** CustomTypes 인덱스 (정의 순서) */
		int32 CustomTypeIndex = INDEX_NONE;
		/** 큰 방 예약 대상 타입인지 (FPMGCustomRoomType::NeedsReservedBigRoom) */
		bool bNeedsReservedBigRoom = false;
		/** 방 에셋이 등록된 크기 수 (0 ~ 3) */
		int32 RegisteredSizeCount = 0;
		/** 추가방에만 배정되는 타입인지 (FPMGCustomRoomType::bExtraRoom) */
		bool bExtraRoom = false;
		/** 켠 문 제외 옵션 수 (bAvoidDoorToEntrance, bAvoidDoorToExit 중, 0 ~ 2) */
		int32 AvoidOptionCount = 0;
	};

	/**
	 * ValidCustomTypeIndices를 처리 순서로 정렬한다:
	 * 큰 방 예약 대상 타입 먼저 → 등록된 크기 수가 적은 타입 먼저 → 추가방 전용 타입 먼저 → 문 제외 옵션을 많이 켠 타입 먼저 → 정의 순서.
	 * 마지막 기준(인덱스)이 모두 다르므로 순서가 하나로 정해진다
	 */
	TArray<int32> SortCustomTypesByConstraint(const TArray<FPMGCustomRoomType>& CustomTypes, const TArray<int32>& ValidCustomTypeIndices)
	{
		TArray<FCustomTypeOrder> Orders;
		for (const int32 CustomTypeIndex : ValidCustomTypeIndices)
		{
			const FPMGCustomRoomType& CustomType = CustomTypes[CustomTypeIndex];
			FCustomTypeOrder& Order = Orders.AddDefaulted_GetRef();
			Order.CustomTypeIndex = CustomTypeIndex;
			Order.bNeedsReservedBigRoom = CustomType.NeedsReservedBigRoom();
			Order.RegisteredSizeCount = CustomType.Assets.GetRegisteredRoomSizes().CountSizes();
			Order.bExtraRoom = CustomType.bExtraRoom;
			Order.AvoidOptionCount = (CustomType.bAvoidDoorToEntrance ? 1 : 0) + (CustomType.bAvoidDoorToExit ? 1 : 0);
		}

		Orders.Sort([](const FCustomTypeOrder& A, const FCustomTypeOrder& B)
		{
			if (A.bNeedsReservedBigRoom != B.bNeedsReservedBigRoom)
			{
				return A.bNeedsReservedBigRoom;   // 예약 대상이 앞
			}
			if (A.RegisteredSizeCount != B.RegisteredSizeCount)
			{
				return A.RegisteredSizeCount < B.RegisteredSizeCount;
			}
			if (A.bExtraRoom != B.bExtraRoom)
			{
				// 크기가 같으면 추가방 전용 타입의 대상(추가방만)은 일반 타입 대상의 일부이므로 먼저 고른다
				return A.bExtraRoom;
			}
			if (A.AvoidOptionCount != B.AvoidOptionCount)
			{
				// 위 기준이 모두 같으면 옵션을 더 많이 켠 타입의 대상은 적게 켠 타입 대상의 일부이므로 먼저 고른다
				// (옵션이 두 개뿐이라, 더 많이 켰으면 상대가 켠 옵션도 모두 켠 것)
				return A.AvoidOptionCount > B.AvoidOptionCount;
			}
			return A.CustomTypeIndex < B.CustomTypeIndex;
		});

		TArray<int32> SortedIndices;
		for (const FCustomTypeOrder& Order : Orders)
		{
			SortedIndices.Add(Order.CustomTypeIndex);
		}
		return SortedIndices;
	}

	/** 방마다 RoomId 방과 문으로 바로 이어져 있는지 (인덱스 = 방 ID) */
	TArray<bool> FindRoomsWithDoorTo(const FPMGLayout& Layout, int32 RoomId)
	{
		TArray<bool> bHasDoor;
		bHasDoor.Init(false, Layout.Rooms.Num());
		for (const FPMGDoor& Door : Layout.Doors)
		{
			if (Door.RoomA == RoomId)
			{
				bHasDoor[Door.RoomB] = true;
			}
			if (Door.RoomB == RoomId)
			{
				bHasDoor[Door.RoomA] = true;
			}
		}
		return bHasDoor;
	}
}

bool PMG::AssignCustomRoomTypes(FPMGLayout& Layout, const TArray<FPMGCustomRoomType>& CustomTypes, const TArray<int32>& ValidCustomTypeIndices,
	const FPMGRoomTypeAssets& StandardRooms, FRandomStream& Stream, FString& OutError)
{
	const FPMGRoomSizeSet StandardRoomSizes = StandardRooms.GetRegisteredRoomSizes();
	// 문은 이 단계에서 바뀌지 않으므로 한 번만 구한다
	const TArray<bool> bHasDoorToEntrance = FindRoomsWithDoorTo(Layout, Layout.EntranceRoomId);
	const TArray<bool> bHasDoorToExit = FindRoomsWithDoorTo(Layout, Layout.ExitRoomId);

	for (const int32 CustomTypeIndex : SortCustomTypesByConstraint(CustomTypes, ValidCustomTypeIndices))
	{
		const FPMGCustomRoomType& CustomType = CustomTypes[CustomTypeIndex];
		if (CustomType.Count <= 0)
		{
			continue;
		}
		const FPMGRoomSizeSet TypeRoomSizes = CustomType.Assets.GetRegisteredRoomSizes();

		// 1. 대상 방을 둘로 나눈다 (방 ID 오름차순).
		//    StandardRooms에 그 크기 에셋이 없는 방은 Standard로 남으면 스폰되지 않으므로 먼저 고른다 (2-9절 "방 선택 우선순위")
		TArray<int32> PriorityRoomIds;
		TArray<int32> OtherRoomIds;
		for (const FPMGRoom& Room : Layout.Rooms)
		{
			const bool bTarget = Room.bConnected
				&& Room.RoomType == PMG::RoomTypeStandard()   // 입구/출구와 이미 다른 커스텀 타입이 된 방은 제외
				&& TypeRoomSizes.Contains(Room.Shape)
				&& (!CustomType.bExtraRoom || Room.bExtra)
				&& !(CustomType.bAvoidDoorToEntrance && bHasDoorToEntrance[Room.Id])
				&& !(CustomType.bAvoidDoorToExit && bHasDoorToExit[Room.Id]);
			if (!bTarget)
			{
				continue;
			}
			if (StandardRoomSizes.Contains(Room.Shape))
			{
				OtherRoomIds.Add(Room.Id);
			}
			else
			{
				PriorityRoomIds.Add(Room.Id);
			}
		}

		// 2. 각각 섞어 우선 방 → 나머지 방 순서로 잇는다.
		//    고를 수가 대상 수 이상이어도 섞는다 (난수 사용 횟수가 Count에 따라 달라지지 않게)
		Algo::RandomShuffle(PriorityRoomIds, Stream);
		Algo::RandomShuffle(OtherRoomIds, Stream);
		TArray<int32> OrderedRoomIds = PriorityRoomIds;
		OrderedRoomIds.Append(OtherRoomIds);

		// 3. 대상이 모자라면 시도 실패 (1-2절 ③). 원하는 설정대로 만들지 못한 맵을 성공으로 내보내지 않는다
		if (OrderedRoomIds.Num() < CustomType.Count)
		{
			FString TargetText = TEXT("사용된 Standard 방 중 이 타입에 방 에셋이 등록된 크기의 방");
			if (CustomType.bExtraRoom)
			{
				TargetText += TEXT(", 추가방만");
			}
			if (CustomType.bAvoidDoorToEntrance)
			{
				TargetText += TEXT(", 입구방과 문으로 이어진 방 제외");
			}
			if (CustomType.bAvoidDoorToExit)
			{
				TargetText += TEXT(", 출구방과 문으로 이어진 방 제외");
			}
			OutError = FString::Printf(TEXT("CustomTypes[%d] '%s': 배정할 수 있는 방 %d개 < Count %d (대상 = %s)"),
				CustomTypeIndex, *CustomType.TypeName.ToString(), OrderedRoomIds.Num(), CustomType.Count, *TargetText);
			return false;
		}

		// 4. 앞에서부터 Count개
		for (int32 Index = 0; Index < CustomType.Count; ++Index)
		{
			Layout.Rooms[OrderedRoomIds[Index]].RoomType = CustomType.TypeName;
		}
	}
	return true;
}
