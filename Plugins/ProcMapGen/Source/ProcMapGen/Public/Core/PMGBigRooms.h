#pragma once

#include "CoreMinimal.h"
#include "Core/PMGLayout.h"

namespace PMG
{
	/**
	 * 추가방 전용 타입에 쓸 큰 방을 예약한다 (2-9절 "추가방용 큰 방 예약"). 예약한 큰 방은 루트 위에서 만들지 않고,
	 * Phase 6에서 루트 옆에 추가방으로 붙인다.
	 * - ReservedRoomSizeChoices: 예약할 방마다 고를 수 있는 큰 크기 (bDual/bQuad). UPMGSettings::Validate() 결과의 값
	 * - AvailableQuadRoomCount/AvailableDualRoomCount: 예약에 쓸 수 있는 4칸/2칸 방 수 (설정 개수에서 입구/출구가 쓴 것을 뺀 수)
	 * 방마다 앞에서부터: 두 크기 모두 남아 있으면 무작위, 하나만 남아 있으면 그 크기, 둘 다 없으면 예약하지 않는다.
	 */
	PROCMAPGEN_API void ChooseReservedBigRooms(const TArray<FPMGRoomSizeSet>& ReservedRoomSizeChoices, int32 AvailableQuadRoomCount, int32 AvailableDualRoomCount,
		FRandomStream& Stream, int32& OutReservedQuadRoomCount, int32& OutReservedDualRoomCount);

	/**
	 * Phase 4: 루트 위의 1칸 방들을 합쳐 큰 방을 만든다 (2-7절 "루트 위 큰 방 합치기"). 루트가 큰 방을 관통하게 된다.
	 *
	 * 합칠 수 있는 자리 (맵 안의 아무 위치. 짝수 정렬 같은 제한은 없다):
	 * - 4칸: 2x2 칸이 모두 합칠 수 있는 1칸 방(입구/출구가 아니고 아직 합치지 않음)이고, 그중 루트 칸이 2개 이상.
	 *        나머지 칸은 루트 밖이어도 된다 (방이 루트 옆으로 넓어진다)
	 * - 2칸: 가로/세로로 맞닿은 두 칸이 모두 합칠 수 있는 1칸 방이고, 둘 다 루트 칸
	 * - 공통: 루트 칸들의 소속(루트 1만 / 루트 2만 / 두 루트 공통)이 같고, 각 루트에서 순서가 연속
	 *         (떨어진 칸을 합치면 루트가 같은 방을 두 번 지나고, 소속이 다른 칸을 합치면 두 루트가 붙는다)
	 * 4칸을 먼저 QuadRoomCount개, 그다음 2칸을 DualRoomCount개 고른다. 자리마다 best-candidate로 이미 놓인 큰 방과
	 * 입구/출구에서 먼 자리를 고르며 (SpreadCandidateCount = 표본 수), 고른 자리와 겹치는 후보는 지운다.
	 * 자리가 모자라 만들지 못한 수는 OutLeftoverQuadRoomCount/OutLeftoverDualRoomCount로 돌려준다 (Phase 6이 루트 옆에 붙인다).
	 *
	 * 합친 뒤 방 ID가 바뀌므로 입구/출구, 루트, 이웃을 다시 맞춘다 (PMG::MergeRooms).
	 * 시도 실패: 합친 뒤 "루트 방 수 + 남은 큰 방 수 + ReservedRoomCount(추가방 예약)"가 TotalRoomCount를 넘으면
	 * Layout을 바꾸지 않고 OutError에 사유를 쓴 뒤 false. 성공하면 Layout을 바꾸고 true.
	 *
	 * Layout에는 루트(Phase 4 PMG::FindRoutes)가 정해져 있고, 입구/출구를 뺀 모든 방이 1칸이어야 한다.
	 */
	PROCMAPGEN_API bool MergeRouteBigRooms(FPMGLayout& Layout, int32 QuadRoomCount, int32 DualRoomCount, int32 ReservedRoomCount, int32 TotalRoomCount,
		int32 SpreadCandidateCount, FRandomStream& Stream, int32& OutLeftoverQuadRoomCount, int32& OutLeftoverDualRoomCount, FString& OutError);

	/**
	 * Phase 6 [A]: 루트 방 바로 옆에 큰 방을 붙인다 (2-8절 "루트 옆 큰 방"). 루트 위에 만들지 못한 큰 방과 추가방용으로 예약한 큰 방이 대상이다.
	 *
	 * 붙일 수 있는 자리: 칸이 모두 미사용(bConnected가 false) 1칸 방인 2x2 (4칸) / 가로 또는 세로로 맞닿은 두 칸 (2칸)이고,
	 * 그중 한 칸이라도 루트 방(FPMGRoom::IsRouteRoom)과 맞닿은 자리.
	 * 4칸을 먼저 QuadRoomCount개, 그다음 2칸을 DualRoomCount개 고른다. 고르는 방법은 루트 위 큰 방과 같다 (best-candidate:
	 * 이미 놓인 큰 방과 입구/출구에서 먼 자리, SpreadCandidateCount = 표본 수, 고른 자리와 겹치는 후보는 지움).
	 * 고른 자리를 모두 합친 뒤(PMG::MergeRooms), 큰 방마다 맞닿은 루트 방 중 하나를 무작위로 골라 문으로 잇는다
	 * (PMG::ConnectRooms, RoomA = 루트 방). 붙인 큰 방은 추가방(bExtra)이다.
	 *
	 * 시도 실패: 자리가 모자라 4칸이나 2칸을 다 붙일 수 없으면 Layout을 바꾸지 않고 OutError에 사유를 쓴 뒤 false.
	 * 성공하면 Layout을 바꾸고 true.
	 *
	 * Layout에는 루트 연결(Phase 5 PMG::ConnectRoutes)이 끝나 있어야 한다.
	 */
	PROCMAPGEN_API bool AttachBigRoomsBesideRoutes(FPMGLayout& Layout, int32 QuadRoomCount, int32 DualRoomCount, int32 SpreadCandidateCount,
		FRandomStream& Stream, FString& OutError);
}
