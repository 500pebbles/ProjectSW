#pragma once

#include "CoreMinimal.h"
#include "Core/PMGLayout.h"

namespace PMG
{
	/**
	 * 노드 비용 다익스트라: 시작 방에서 출발해 목표 방 중 하나까지 가는 가장 싼 경로.
	 * 경로 비용 = 시작 방을 뺀, 경로 위 모든 방의 진입 비용 합 (목표 방 포함).
	 * 루트 1, 루트 2 (Phase 4)가 쓴다. 시작/목표 방을 여러 개 받을 수 있는 범용 함수다.
	 *
	 * - StartRoomIds: 출발할 수 있는 방. 여러 개면 그중 어디서 출발해도 된다
	 * - TargetRoomIds: 목표 방. 여러 개면 가장 싸게 도착하는 하나까지의 경로를 돌려준다
	 * - RoomEnterCosts: 방 ID마다 그 방에 들어가는 비용 (0 이상)
	 * - bPassableRooms: 방 ID마다 들어갈 수 있는지. false인 방은 지나가지도, 목표로 도착하지도 않는다 (시작 방은 이 값과 무관하게 출발점)
	 *
	 * 비용이 같으면 방 ID가 작은 방부터 처리하므로 같은 입력이면 어느 머신에서든 같은 경로가 나온다 (Roadmap 3-6절).
	 * 반환: 시작 방 → 목표 방 순서의 방 ID (양 끝 포함). 시작 방이 곧 목표 방이면 그 방 하나. 도착할 수 없으면 빈 배열.
	 */
	PROCMAPGEN_API TArray<int32> FindCheapestPath(const FPMGLayout& Layout, const TArray<int32>& StartRoomIds, const TArray<int32>& TargetRoomIds,
		const TArray<int64>& RoomEnterCosts, const TArray<bool>& bPassableRooms);

	/**
	 * Phase 4: 입구방 → 출구방 루트 1, 2를 찾는다 (2-7절 비용 모델).
	 * 루트를 찾을 때는 입구/출구를 뺀 모든 방이 1칸 방이다. 큰 방은 루트를 찾은 뒤 루트 위에서 만든다 (PMG::MergeRouteBigRooms).
	 * 1. 루트 1: 모든 방 비용 1 → 지나는 방이 가장 적은 길 중 하나
	 * 2. 루트 2: 루트 1에 든 방은 RouteCostPenalty, 나머지는 1 → 루트 1을 피해 돌아가는 길
	 * 비용은 방마다 "설정 비용 x (100 ± 10)" 정수다 (100배 정수 단위 + ±10% 지터). 지터는 루트마다 새로 뽑는다.
	 *
	 * 다음 경우는 시도 실패: Layout을 바꾸지 않고 OutError에 사유를 쓴 뒤 false
	 * - 입구방에서 출구방까지 경로 없음
	 * - 루트 2가 루트 1과 같음 (우회로가 없는 병목 구조)
	 * 성공하면 Layout의 PrimaryRoute/SecondaryRoute를 채우고 true.
	 * 루트 방 수가 TotalRoomCount를 넘는지는 PMG::MergeRouteBigRooms에서 검사한다 (합치면 방 수가 줄어든다).
	 *
	 * Layout에는 이웃(Phase 2)과 입구/출구(Phase 3)가 정해져 있어야 한다.
	 * RouteCostPenalty는 UPMGSettings::Validate()로 보정된 값을 넣는다.
	 */
	PROCMAPGEN_API bool FindRoutes(FPMGLayout& Layout, int32 RouteCostPenalty, FRandomStream& Stream, FString& OutError);
}
