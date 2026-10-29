#include "SoulGameplayTags.h"

namespace SoulTags
{
	namespace Attributes
	{
		UE_DEFINE_GAMEPLAY_TAG(Health, "SoulTags.Attributes.Health")
		UE_DEFINE_GAMEPLAY_TAG(Stamina, "SoulTags.Attributes.Stamina")		
		UE_DEFINE_GAMEPLAY_TAG(MaxHealth, "SoulTags.Attributes.MaxHealth")
		UE_DEFINE_GAMEPLAY_TAG(MaxStamina, "SoulTags.Attributes.MaxStamina")
		UE_DEFINE_GAMEPLAY_TAG(HealthRegeneration, "SoulTags.Attributes.HealthRegeneration")
		UE_DEFINE_GAMEPLAY_TAG(StaminaRegeneration, "SoulTags.Attributes.StaminaRegeneration")
		UE_DEFINE_GAMEPLAY_TAG(AttackPower, "SoulTags.Attributes.AttackPower")
		UE_DEFINE_GAMEPLAY_TAG(Defense, "SoulTags.Attributes.Defense")
		UE_DEFINE_GAMEPLAY_TAG(CriticalRate, "SoulTags.Attributes.CriticalRate")
		UE_DEFINE_GAMEPLAY_TAG(CriticalDamage, "SoulTags.Attributes.CriticalDamage")
		UE_DEFINE_GAMEPLAY_TAG(CooldownReduction, "SoulTags.Attributes.CooldownReduction")
		UE_DEFINE_GAMEPLAY_TAG(DamageAmplification, "SoulTags.Attributes.DamageAmplification")
	}
	
	namespace Combats
	{
		UE_DEFINE_GAMEPLAY_TAG(Damage, "SoulTags.Combats.Damage")
		UE_DEFINE_GAMEPLAY_TAG(HitReact, "SoulTags.Combats.HitReact")
	}
	
	namespace Abilities
	{
		UE_DEFINE_GAMEPLAY_TAG(Primary, "SoulTags.Abilities.Primary")
	} 
	
	namespace States
	{
		
	}
	
	namespace InputTags
	{
		UE_DEFINE_GAMEPLAY_TAG(LMB, "SoulTags.InputTags.LMB")
		UE_DEFINE_GAMEPLAY_TAG(RMB, "SoulTags.InputTags.RMB")
		UE_DEFINE_GAMEPLAY_TAG(KEY_1, "SoulTags.InputTags.KEY_1")
		UE_DEFINE_GAMEPLAY_TAG(KEY_2, "SoulTags.InputTags.KEY_2")
		UE_DEFINE_GAMEPLAY_TAG(KEY_3, "SoulTags.InputTags.KEY_3")
		UE_DEFINE_GAMEPLAY_TAG(KEY_4, "SoulTags.InputTags.KEY_4")
	}
	
	namespace Events
	{
		namespace Montages
		{
			UE_DEFINE_GAMEPLAY_TAG(Fireball, "SoulTags.Events.Montages.Fireball")
		}
	}
}