#pragma once

#include "CoreMinimal.h"
#include "Data/PMGTypes.h"

namespace PMG
{
	/** 셀 범위 [Min, Max)의 중심 (셀 단위). 예: 셀 (0, 0)은 (0.5, 0.5), [0,2)x[0,2)는 (1, 1). 방 중심과 아직 방이 아닌 자리의 중심 모두 이 식을 쓴다 */
	inline FVector2D GetRectCenter(const FIntRect& Bounds)
	{
		return FVector2D((Bounds.Min.X + Bounds.Max.X) * 0.5, (Bounds.Min.Y + Bounds.Max.Y) * 0.5);
	}

	/** 두 셀 범위 [Min, Max)가 셀을 하나라도 같이 덮는지. 변만 맞닿은 것은 겹친 것이 아니다 (Max == 다른 쪽 Min이면 떨어져 있음) */
	inline bool DoRectsOverlap(const FIntRect& A, const FIntRect& B)
	{
		return A.Min.X < B.Max.X && B.Min.X < A.Max.X
			&& A.Min.Y < B.Max.Y && B.Min.Y < A.Max.Y;
	}
}

/** 방 (2-3절). 이후 Phase에서 필드가 추가된다 */
struct FPMGRoom
{
	int32 Id = INDEX_NONE;
	EPMGRoomShape Shape = EPMGRoomShape::Single;

	/** 방이 차지하는 셀 범위 [Min, Max). X = Col, Y = Row */
	FIntRect Bounds;

	/** 상하좌우로 맞닿은 다른 방의 ID. 오름차순, 중복 없음 (Phase 2 PMG::BuildNeighbors에서 채움) */
	TArray<int32> Neighbors;

	/**
	 * 방 타입 (2-9절). 기본 Standard, 입구방은 Entrance, 출구방은 Exit (Phase 3 PMG::ChooseEntranceAndExit에서 정함).
	 * 사용된 Standard 방 일부는 커스텀 타입 이름이 된다 (Phase 7 PMG::AssignCustomRoomTypes)
	 */
	FName RoomType = PMG::RoomTypeStandard();

	/** 사용(연결)되는 방인지. 문으로 다른 방과 이어지면 true (Phase 5 PMG::ConnectRooms에서 정함) */
	bool bConnected = false;

	/**
	 * 추가방인지: 루트에 없고, 루트 방 옆에 붙여 연결한 방 (Phase 6 PMG::AttachBigRoomsBesideRoutes, PMG::FillBranches에서 정함).
	 * true면 bConnected도 true다
	 */
	bool bExtra = false;

	/**
	 * 배정된 방 에셋의 인덱스.
	 * - 사용된 방: 이 방의 타입과 크기에 맞는 에셋 리스트(FPMGRoomTypeAssets::GetRoomAssets)의 인덱스 (Phase 9 PMG::AssignRoomAssets에서 정함)
	 * - 미사용 방: 채움 방 리스트(FPMGAssetLists::FillerRooms)의 인덱스 (Phase 10 PMG::AssignFillerAssets에서 정함, 미사용 방 정책이 SpawnFiller일 때만)
	 * INDEX_NONE이면 스폰하지 않는 방이다: 리스트가 비어 있는 사용된 방, 또는 채움 방을 쓰지 않는 미사용 방
	 */
	int32 VariantIndex = INDEX_NONE;

	/** 배정된 에셋의 회전: 0, 1, 2, 3 → 0/90/180/270° (Phase 9, 10). 2칸 방은 0 또는 2. 회전을 허용하지 않는 에셋이면 0 */
	uint8 Rotation = 0;

	/** 루트 방인지 (루트 1/2에 든 방, 입구/출구 포함) = 연결되었고 추가방이 아님 (2-3절). 루트 연결(Phase 5) 뒤에 의미가 있다 */
	bool IsRouteRoom() const
	{
		return bConnected && !bExtra;
	}

	/** 방 중심 (셀 단위). 예: 셀 (0, 0)인 1칸 방은 (0.5, 0.5), [0,2)x[0,2)인 4칸 방은 (1, 1) */
	FVector2D GetCenter() const
	{
		return PMG::GetRectCenter(Bounds);
	}
};

/**
 * 두 방을 잇는 문 (Phase 5). 문은 맞닿은 두 셀 CellA와 CellB 사이의 경계에 놓인다.
 * CellA는 RoomA의 셀, CellB는 RoomB의 셀이며 상하좌우로 맞닿아 있다.
 */
struct FPMGDoor
{
	int32 RoomA = INDEX_NONE;
	int32 RoomB = INDEX_NONE;
	FIntPoint CellA = FIntPoint::NoneValue;
	FIntPoint CellB = FIntPoint::NoneValue;
};

/**
 * 커넥터 (3-4절, Phase 9): 셀 경계(엣지) 하나를 채우는 문 또는 벽. 맞닿은 두 셀 CellA와 CellB 사이의 경계에 놓인다.
 * 스폰되는 방(FPMGRoom::VariantIndex가 있는 방)이 하나라도 접한 경계마다 하나씩 있다.
 */
struct FPMGConnector
{
	/**
	 * 경계 양쪽의 셀. CellA는 항상 맵 안이고, CellB는 맵 테두리면 맵 밖이다.
	 * 문이면 문(FPMGDoor)의 CellA/CellB 그대로 (RoomA → RoomB 방향), 그 밖에는 CellB가 CellA의 +X 또는 +Y 쪽 (맵 테두리는 바깥쪽)
	 */
	FIntPoint CellA = FIntPoint::NoneValue;
	FIntPoint CellB = FIntPoint::NoneValue;

	EPMGConnectorType Type = EPMGConnectorType::Wall;

	/** 문 에셋을 가져온 방 타입: 그 타입의 Doors (3-4절 "문 에셋 고르기"). NAME_None이면 전역 Doors. 벽이면 NAME_None */
	FName DoorListRoomType;

	/** 배정된 에셋: 문은 DoorListRoomType에 해당하는 문 리스트, 벽은 Walls의 인덱스. 리스트가 비어 있으면 INDEX_NONE (문이면 개구부만 열림, 벽이면 막이 없음) */
	int32 VariantIndex = INDEX_NONE;
};

/** 맵의 방 구조 (Phase 1), 방 이웃 정보 (Phase 2), 입구/출구 (Phase 3), 루트 1/2와 루트 위 큰 방 (Phase 4), 문 (Phase 5), 추가방 (Phase 6), 방 타입 (Phase 7), 에셋과 커넥터 (Phase 9) */
struct PROCMAPGEN_API FPMGLayout
{
	int32 Rows = 0;
	int32 Cols = 0;

	/** 방 목록. 방 ID = 배열 인덱스. 방의 왼쪽 위 셀 기준으로 위쪽 줄부터, 왼쪽부터 번호를 붙인다 */
	TArray<FPMGRoom> Rooms;

	/** 셀마다 그 셀이 속한 방 ID. 2차원 격자를 1차원으로 펼친 배열이며 인덱스 = Y * Cols + X */
	TArray<int32> CellToRoom;

	/** 입구방 ID. Phase 3 PMG::ChooseEntranceAndExit에서 정하며, 정하기 전에는 INDEX_NONE */
	int32 EntranceRoomId = INDEX_NONE;

	/** 출구방 ID. Phase 3 PMG::ChooseEntranceAndExit에서 정하며, 정하기 전에는 INDEX_NONE */
	int32 ExitRoomId = INDEX_NONE;

	/** 루트 1: 입구방 → 출구방 순서의 방 ID (양 끝 포함). Phase 4 PMG::FindRoutes에서 정하며, 정하기 전에는 비어 있음 */
	TArray<int32> PrimaryRoute;

	/** 루트 2: 루트 1에 든 방을 피해서 찾은 두 번째 경로 (2-7절). 형식은 PrimaryRoute와 같다 */
	TArray<int32> SecondaryRoute;

	/** 방 사이의 문. 방 쌍마다 하나뿐이다 (Phase 5 PMG::ConnectRoutes에서 루트의 방 쌍마다, Phase 6에서 추가방마다 하나씩 만든다) */
	TArray<FPMGDoor> Doors;

	/** 커넥터 (문/벽). Phase 9 PMG::BuildConnectors에서 만들고 PMG::AssignConnectorAssets에서 에셋을 정한다 */
	TArray<FPMGConnector> Connectors;

	int32 GetRoomIdAt(const FIntPoint& Cell) const { return CellToRoom[Cell.Y * Cols + Cell.X]; }

	/** 셀이 맵 안인지 */
	bool IsInsideMap(const FIntPoint& Cell) const
	{
		return Cell.X >= 0 && Cell.X < Cols && Cell.Y >= 0 && Cell.Y < Rows;
	}

	/** 셀 범위 [Min, Max)가 모두 맵 안인지. Max는 범위에 들지 않으므로 Max.X == Cols까지 맵 안이다 */
	bool IsInsideMap(const FIntRect& Bounds) const
	{
		return Bounds.Min.X >= 0 && Bounds.Min.Y >= 0 && Bounds.Max.X <= Cols && Bounds.Max.Y <= Rows;
	}

	/** 방 경계를 그린 텍스트 (줄 단위). 셀 문자는 입구방 E, 출구방 X, 그 밖은 방 칸 수 (1/2/4). 문이 있는 경계는 #. 위쪽 줄이 Y = 0 */
	TArray<FString> ToDebugLines() const;

	/** ToDebugLines와 같은 그림에 루트를 표시한다. 셀 문자는 입구방 E, 출구방 X, 두 루트 공통 B, 루트 1만 1, 루트 2만 2, 추가방 *, 그 밖(미사용) . */
	TArray<FString> ToRouteDebugLines() const;

private:
	/** 방 경계를 그린다. 셀에 찍을 문자는 GetRoomChar(그 셀의 방 ID)로 정하고, 문(Doors)이 있는 경계는 #로 그린다 */
	TArray<FString> DrawRoomMap(TFunctionRef<TCHAR(int32 RoomId)> GetRoomChar) const;
};

namespace PMG
{
	/**
	 * Rows x Cols 맵에 서로 겹치지 않게 놓을 수 있는 4칸 방(2x2)의 최대 수 = floor(Rows/2) * floor(Cols/2).
	 * 2x2 방은 어디에 놓든 "X, Y가 모두 홀수인 셀"을 정확히 하나 덮으므로, 그런 셀 수보다 많이 놓을 수 없다.
	 * 반대로 짝수 좌표마다 하나씩 놓으면 이 수만큼 놓을 수 있다.
	 */
	PROCMAPGEN_API int32 GetMaxQuadRoomCount(int32 Rows, int32 Cols);

	/**
	 * 4칸 방을 QuadRoomCount개 놓은 뒤 더 놓을 수 있는 2칸 방의 최대 수 = floor((Rows*Cols - 4*QuadRoomCount) / 2).
	 * 남은 칸 수의 절반보다 많이 놓을 수는 없다. 반대로 맵을 짝수 좌표마다 2x2 칸으로 나눠 4칸 방을 그 칸들에 놓고,
	 * 남은 2x2 칸마다 2칸 방 2개, 홀수 끝 줄에 2칸 방을 이어 놓으면 이 수만큼 놓을 수 있다 (남는 셀은 많아야 오른쪽 아래 1칸).
	 */
	PROCMAPGEN_API int32 GetMaxDualRoomCount(int32 Rows, int32 Cols, int32 QuadRoomCount);

	/** 4칸 방 QuadRoomCount개와 2칸 방 DualRoomCount개를 맵에 서로 겹치지 않게 놓을 수 있는지 (UPMGSettings::Validate()의 용량 검사) */
	PROCMAPGEN_API bool CanFitBigRooms(int32 Rows, int32 Cols, int32 QuadRoomCount, int32 DualRoomCount);

	/** Phase 1: Rows x Cols 맵의 모든 셀을 1칸 방으로 만든다. OutLayout은 새로 초기화된다 (이웃은 Phase 2 PMG::BuildNeighbors에서) */
	PROCMAPGEN_API void BuildLayout(int32 Rows, int32 Cols, FPMGLayout& OutLayout);

	/**
	 * 1칸 방들을 합쳐 큰 방(NewBigRoomBounds의 사각형 하나 = 방 하나)을 만든다.
	 * 입구/출구 방(Phase 3), 루트 위 큰 방(Phase 4), 루트 옆 큰 방(Phase 6)을 만들 때 쓴다.
	 *
	 * NewBigRoomBounds의 사각형은 2x2, 2x1, 1x2 중 하나이고, 맵 안에 있으며, 지금 1칸 방이고 연결되지 않은(bConnected가 false) 셀만 덮고,
	 * 서로 겹치지 않아야 한다. 연결된 방을 합치면 그 방의 문이 큰 방 안쪽에 남을 수 있기 때문이다.
	 * 방 목록과 CellToRoom을 다시 만들기 때문에 방 ID가 바뀐다. 그래서 다음을 셀 기준으로 다시 맞춘다:
	 * 이미 있던 큰 방, 입구/출구 ID와 그 RoomType, 루트(PrimaryRoute/SecondaryRoute: 같은 방이 연달아 나오면 하나로),
	 * 연결 여부(bConnected), 추가방 여부(bExtra), 문(Doors)의 방 ID, 이웃(Neighbors)
	 * 에셋 배정(Phase 9)은 합치기가 모두 끝난 뒤에 하므로, 에셋(VariantIndex, Rotation)과 커넥터가 있는 Layout은 합치지 않는다
	 */
	PROCMAPGEN_API void MergeRooms(FPMGLayout& Layout, const TArray<FIntRect>& NewBigRoomBounds);

	/** 크기별 방 개수, 면적, CellToRoom 일치를 검사한다 */
	PROCMAPGEN_API bool VerifyLayout(const FPMGLayout& Layout, int32 QuadRoomCount, int32 DualRoomCount, FString& OutError);
}
