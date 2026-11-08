#pragma once

#include "NativeGameplayTags.h"

/** 런 전용 게임플레이 태그 (팀 SoulTags와 분리) */
namespace SoulRunTags
{
	/** 층 전환, 사망, 클리어로 입력이 잠긴 동안 폰에 붙는다. 상호작용과 UI가 이 태그를 보고 막는다 */
	PROJECTSW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Locked);

	/** 클리어가 확정된 뒤 폰에 붙는다 */
	PROJECTSW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Cleared);
}
