#pragma once

#include "CoreMinimal.h"

/**
 * 런 폰 상태 진단 (런 폰을 팀 폰에 접은 뒤 확인용, 11-09).
 * 팀 폰 BP_WorkerPlayerCharacter에 런 컴포넌트가 값 없이 붙어 있고 런 월드에서만 켜지므로,
 * 셀프 테스트와 콘솔 soul.Run.Check가 이 한 줄로 런 월드 여부와 실제 적용 값을 남긴다.
 */
namespace SoulRunCheck
{
	/** [RUNCHECK] 한 줄. 런 월드인데 폰, Equipment, Progression, 카탈로그가 없거나 카메라 랙이 DA_CombatFeel과 다르면 FAIL(SW_LOG_ERROR)을 남기고 false. Shipping은 true만 돌려준다 */
	PROJECTSW_API bool Log(const UObject* WorldContextObject, const TCHAR* Where);
}
