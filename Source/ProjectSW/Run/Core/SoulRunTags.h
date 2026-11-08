#pragma once

#include "NativeGameplayTags.h"

/** 런 전용 게임플레이 태그 (팀 SoulTags와 분리) */
namespace SoulRunTags
{
	/** 층 전환, 사망, 클리어로 입력이 잠긴 동안 폰에 붙는다. 상호작용과 UI가 이 태그를 보고 막는다 */
	PROJECTSW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Locked);

	/** 클리어가 확정된 뒤 폰에 붙는다 */
	PROJECTSW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Cleared);

	/** 회복 GE의 SetByCaller (포션, 회복 이벤트, 층 이동 회복) */
	PROJECTSW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magnitude_Heal);

	/** 함정과 지뢰 피해 GE의 SetByCaller */
	PROJECTSW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magnitude_Trap);
}
