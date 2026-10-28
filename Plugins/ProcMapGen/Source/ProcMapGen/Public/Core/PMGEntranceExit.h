#pragma once

#include "CoreMinimal.h"
#include "Core/PMGLayout.h"

namespace PMG
{
	/** 두 방 중심 사이의 맨해튼 거리 (셀 단위). 입구/출구 거리 기준 (Roadmap 8절 4번) */
	PROCMAPGEN_API double GetRoomDistance(const FPMGRoom& RoomA, const FPMGRoom& RoomB);

	/**
	 * Rows x Cols 맵에서 EntranceSize 크기 방과 ExitSize 크기 방의 중심이 떨어질 수 있는 최대 맨해튼 거리.
	 * 모양(1칸 / 4칸 / 가로 2칸 / 세로 2칸)마다 중심이 놓일 수 있는 범위를 보고 계산한다. 두 방이 겹치는지는 따지지 않는다.
	 * 맵에 들어가지 않는 크기면 (예: 1행 맵의 4칸 방) -1.
	 */
	PROCMAPGEN_API double GetMaxRoomDistance(int32 Rows, int32 Cols, EPMGRoomSize EntranceSize, EPMGRoomSize ExitSize);

	/**
	 * 입구/출구가 될 수 있는 (입구 크기, 출구 크기) 조합 목록 (UPMGSettings::Validate()에서 사용, 2-9절).
	 * EntranceRoomSizes x ExitRoomSizes 중 다음을 만족하는 조합만 1칸 → 2칸 → 4칸 순서로 담는다.
	 * - 방 수: 입구/출구에 필요한 4칸 방 수 <= QuadRoomCount, 2칸 방 수 <= DualRoomCount, 1칸 방 수 <= 남는 1칸 방 수
	 * - 거리: GetMaxRoomDistance >= MinEntranceExitDistance (넘으면 어떤 시도로도 만족할 수 없음)
	 */
	PROCMAPGEN_API TArray<FPMGEntranceExitSizes> MakeEntranceExitSizeCombos(int32 Rows, int32 Cols, int32 QuadRoomCount, int32 DualRoomCount,
		const FPMGRoomSizeSet& EntranceRoomSizes, const FPMGRoomSizeSet& ExitRoomSizes, int32 MinEntranceExitDistance);

	/** 이번 시도의 입구/출구 크기 조합을 SizeCombos 중에서 무작위로 고른다. SizeCombos는 비어 있으면 안 된다 */
	PROCMAPGEN_API FPMGEntranceExitSizes ChooseEntranceExitSizes(const TArray<FPMGEntranceExitSizes>& SizeCombos, FRandomStream& Stream);

	/**
	 * 입구/출구 거리 목표 = max(MinEntranceExitDistance, RouteBigRoomCount)를 맵에서 가능한 최대 거리 이하로 줄인 값.
	 * RouteBigRoomCount = 루트 위에서 만들 큰 방 수 (Phase 4 PMG::MergeRouteBigRooms). 입구/출구를 멀리 떨어뜨려
	 * 루트에 큰 방을 만들 칸을 남긴다. 큰 방 하나가 루트 칸을 2개 이상 쓰고 루트가 2개이므로, 거리가 큰 방 수만큼이면 대략 충분하다.
	 * Sizes는 MakeEntranceExitSizeCombos의 조합이어야 한다 (그래야 결과가 MinEntranceExitDistance 이상).
	 */
	PROCMAPGEN_API int32 GetTargetEntranceExitDistance(int32 Rows, int32 Cols, const FPMGEntranceExitSizes& Sizes, int32 MinEntranceExitDistance, int32 RouteBigRoomCount);

	/**
	 * Phase 3: 입구방과 출구방의 자리를 정하고 그 자리에서 방을 만든다 (2-9절 "입구/출구 배정").
	 * 1. 입구 후보 = Sizes.EntranceSize 크기의 방이 놓일 수 있는 모든 자리, 출구 후보도 같다
	 *    (1칸 = 모든 셀, 4칸 = 맵 안의 모든 2x2, 2칸 = 모든 가로 두 칸과 세로 두 칸)
	 * 2. 후보가 적은 쪽을 먼저 정한다 (같으면 입구 먼저). 그 쪽 후보를 섞어 차례로 보고,
	 *    거리 >= TargetDistance 이고 겹치지 않는 다른 쪽 자리(짝)가 있는 첫 후보를 쓴다
	 * 3. 나머지 쪽은 그 후보의 짝 중 무작위
	 * 4. 고른 자리가 큰 방이면 PMG::MergeRooms로 합친다
	 * 성공하면 Layout의 EntranceRoomId/ExitRoomId와 두 방의 RoomType(Entrance/Exit)을 채우고 true.
	 * 어느 후보에도 짝이 없으면 Layout을 바꾸지 않고 OutError에 사유를 쓴 뒤 false (시도 실패).
	 * Layout은 PMG::BuildLayout으로 만든, 모든 방이 1칸인 상태여야 한다.
	 */
	PROCMAPGEN_API bool ChooseEntranceAndExit(FPMGLayout& Layout, const FPMGEntranceExitSizes& Sizes, int32 TargetDistance,
		FRandomStream& Stream, FString& OutError);
}
