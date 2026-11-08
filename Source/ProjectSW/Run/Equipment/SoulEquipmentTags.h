#pragma once

#include "NativeGameplayTags.h"

/** 플러그인 EquipmentTag.ini가 Config/Tags 밖이라 등록이 불확실해, 같은 태그를 네이티브로 한 번 더 정의한다 */
namespace SoulEquipmentTags
{
	PROJECTSW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Weapon);
}
