#include "GAS/GA/SoulSkillCooldownEffect.h"
#include "Utility/SoulGameplayTags.h"

USoulSkillCooldownEffect::USoulSkillCooldownEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = SoulTags::Cooldown::Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
}
