#pragma once

#include "CoreMinimal.h"

/**
 * 사람 확인용 스크린샷 (셀프 테스트 전용). -SoulShotDir=<폴더>가 있을 때만 켜진다.
 * 화면(UI 포함)을 <폴더>/<이름>.png로 남긴다. 지연은 코어 티커라 맵 이동을 넘어간다.
 */
namespace SoulShots
{
	PROJECTSW_API bool IsEnabled();
	PROJECTSW_API void Take(const FString& Name, float DelaySec = 0.f);
}
