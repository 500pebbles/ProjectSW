#pragma once

#include "CoreMinimal.h"
#include "Core/PMGLayout.h"

namespace PMG
{
	/**
	 * 방 RoomA와 RoomB 사이에 문을 놓을 수 있는 자리(문 후보) 목록. 문 후보 = 상하좌우로 맞닿은 셀 쌍 (CellA는 RoomA의 셀, CellB는 RoomB의 셀).
	 * RoomA의 셀을 위쪽 줄부터, 왼쪽부터 보고, 셀마다 +X, +Y, -X, -Y 순서로 옆 셀을 본다 (방이 최대 2x2라 16번 이하).
	 * 두 방이 맞닿아 있지 않으면 빈 배열.
	 */
	PROCMAPGEN_API TArray<FPMGDoor> GetDoorCandidates(const FPMGLayout& Layout, int32 RoomA, int32 RoomB);

	/**
	 * 방 RoomA와 RoomB를 문 하나로 잇는다: 문 후보 중 하나를 무작위로 골라 Layout.Doors에 더하고, 두 방의 bConnected를 true로 한다.
	 * 두 방 사이에 이미 문이 있으면(RoomA, RoomB 순서와 무관) 아무것도 하지 않는다 → 방 쌍마다 문은 하나뿐이다.
	 * 두 방은 이웃이어야 한다. Phase 5 루트 연결(PMG::ConnectRoutes)이 쓴다.
	 */
	PROCMAPGEN_API void ConnectRooms(FPMGLayout& Layout, int32 RoomA, int32 RoomB, FRandomStream& Stream);

	/**
	 * Phase 5: 루트 1, 루트 2에서 연속된 방 쌍마다 문을 만든다 (PMG::ConnectRooms). 루트 1을 먼저, 그다음 루트 2.
	 * 두 루트가 같은 방 쌍을 지나면 문은 루트 1에서 만든 하나뿐이다. 루트에 든 방은 모두 bConnected가 된다.
	 * 루트에서 연속된 두 방은 항상 이웃이므로 실패하지 않는다.
	 *
	 * Layout에는 루트와 루트 위 큰 방(Phase 4 PMG::FindRoutes, PMG::MergeRouteBigRooms)이 정해져 있어야 한다.
	 * 문을 만든 뒤에도 큰 방을 합칠 수 있다 (PMG::MergeRooms가 문의 방 ID를 다시 맞춘다). 단, 연결된 방은 합칠 수 없다.
	 */
	PROCMAPGEN_API void ConnectRoutes(FPMGLayout& Layout, FRandomStream& Stream);
}
