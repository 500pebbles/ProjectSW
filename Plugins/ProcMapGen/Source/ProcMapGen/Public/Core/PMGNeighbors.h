#pragma once

#include "CoreMinimal.h"
#include "Core/PMGLayout.h"

namespace PMG
{
	/**
	 * Phase 2: 방 이웃 설정.
	 * BuildLayout으로 만든 Layout의 CellToRoom을 보고, 상하좌우로 맞닿은 서로 다른 방을 이웃으로 등록한다 (FPMGRoom::Neighbors, 대각선 제외).
	 * 난수를 쓰지 않으므로 같은 Layout이면 항상 같은 결과가 나온다.
	 */
	PROCMAPGEN_API void BuildNeighbors(FPMGLayout& Layout);
}
