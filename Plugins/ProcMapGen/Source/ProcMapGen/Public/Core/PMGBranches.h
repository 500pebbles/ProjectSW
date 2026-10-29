#pragma once

#include "CoreMinimal.h"
#include "Core/PMGLayout.h"

namespace PMG
{
	/**
	 * Phase 6 [B]: 분기 채우기 (2-8절). 연결된 방이 TotalRoomCount개가 될 때까지 루트 방 옆의 미사용 1칸 방을 하나씩 추가방으로 붙인다.
	 * 붙일 방 수(예산) = TotalRoomCount - 지금 연결된 방 수 (루트 방 + 루트 옆 큰 방).
	 *
	 * 하나 붙일 때마다:
	 * 1. 프런티어 = 미사용 이웃이 있는 루트 방 (FPMGRoom::IsRouteRoom). 추가방은 프런티어가 아니다 (깊이 1)
	 * 2. 프런티어 중 하나를 무작위로 고르고, 그 방의 미사용 이웃 중 하나를 무작위로 고른다
	 * 3. 두 방을 문으로 잇고 (PMG::ConnectRooms, RoomA = 루트 방), 붙인 방은 추가방(bExtra)이 된다
	 *
	 * 시도 실패: 루트 방에 맞닿은 미사용 방이 예산보다 적으면 (= 붙이는 도중에 프런티어가 빔)
	 * Layout을 바꾸지 않고 OutError에 사유를 쓴 뒤 false. 성공하면 Layout을 바꾸고 true.
	 *
	 * Layout에는 루트 연결(Phase 5 PMG::ConnectRoutes)과 루트 옆 큰 방(PMG::AttachBigRoomsBesideRoutes)이 끝나 있어야 한다.
	 * 그러면 미사용 방은 모두 1칸이고, 연결된 방 수는 TotalRoomCount 이하다 (Phase 4 PMG::MergeRouteBigRooms의 방 수 검사).
	 */
	PROCMAPGEN_API bool FillBranches(FPMGLayout& Layout, int32 TotalRoomCount, FRandomStream& Stream, FString& OutError);
}
