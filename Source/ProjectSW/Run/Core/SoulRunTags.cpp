#include "Run/Core/SoulRunTags.h"

namespace SoulRunTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Locked, "Run.State.Locked", "런: 전환, 사망, 클리어로 입력이 잠김");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Cleared, "Run.State.Cleared", "런: 클리어 확정");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Magnitude_Heal, "Run.Magnitude.Heal", "런: 회복량 SetByCaller");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Magnitude_Trap, "Run.Magnitude.Trap", "런: 함정 피해 SetByCaller");
}
