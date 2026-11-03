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
		UE_DEFINE_GAMEPLAY_TAG(PhysicalResistance, "SoulTags.Attributes.PhysicalResistance")
		UE_DEFINE_GAMEPLAY_TAG(SinResistance, "SoulTags.Attributes.SinResistance")
		UE_DEFINE_GAMEPLAY_TAG(VirtueResistance, "SoulTags.Attributes.VirtueResistance")
		UE_DEFINE_GAMEPLAY_TAG(GraceResistance, "SoulTags.Attributes.GraceResistance")
		UE_DEFINE_GAMEPLAY_TAG(HateResistance, "SoulTags.Attributes.HateResistance")
		UE_DEFINE_GAMEPLAY_TAG(TormentResistance, "SoulTags.Attributes.TormentResistance")
		UE_DEFINE_GAMEPLAY_TAG(SolaceResistance, "SoulTags.Attributes.SolaceResistance")
	}
	
	namespace Combats
	{
		UE_DEFINE_GAMEPLAY_TAG(Damage, "SoulTags.Combats.Damage")
		UE_DEFINE_GAMEPLAY_TAG(HitReact, "SoulTags.Combats.HitReact")
	}
	
	namespace DamageTypes
	{
		UE_DEFINE_GAMEPLAY_TAG(Physical, "SoulTags.DamageTypes.Physical")
		UE_DEFINE_GAMEPLAY_TAG(Sin, "SoulTags.DamageTypes.Sin")
		UE_DEFINE_GAMEPLAY_TAG(Virtue, "SoulTags.DamageTypes.Virtue")
		UE_DEFINE_GAMEPLAY_TAG(Grace, "SoulTags.DamageTypes.Grace")
		UE_DEFINE_GAMEPLAY_TAG(Hate, "SoulTags.DamageTypes.Hate")
		UE_DEFINE_GAMEPLAY_TAG(Torment, "SoulTags.DamageTypes.Torment")
		UE_DEFINE_GAMEPLAY_TAG(Solace, "SoulTags.DamageTypes.Solace")
		
		const TMap<FGameplayTag, FGameplayTag>& GetDamageTypes()
		{
			static const TMap<FGameplayTag, FGameplayTag> AllDamageTypes = 
			{
				{ Physical, Attributes::PhysicalResistance}, 
				{ Sin, Attributes::SinResistance},
				{ Virtue, Attributes::VirtueResistance},
				{ Grace, Attributes::GraceResistance},
				{ Hate, Attributes::HateResistance},
				{ Torment, Attributes::TormentResistance},
				{ Solace, Attributes::SolaceResistance}
			};			
			
			return AllDamageTypes;
		}
	}
	
	namespace Abilities
	{
		UE_DEFINE_GAMEPLAY_TAG(Primary, "SoulTags.Abilities.Primary")
		UE_DEFINE_GAMEPLAY_TAG(Attack, "SoulTags.Abilities.Attack")
		UE_DEFINE_GAMEPLAY_TAG(Dash, "SoulTags.Abilities.Dash")
		UE_DEFINE_GAMEPLAY_TAG(Skill, "SoulTags.Abilities.Skill")
	} 
	
	namespace States
	{
		UE_DEFINE_GAMEPLAY_TAG(Dead, "SoulTags.States.Dead")
		UE_DEFINE_GAMEPLAY_TAG(Attacking, "SoulTags.States.Attacking")
		UE_DEFINE_GAMEPLAY_TAG(Dashing, "SoulTags.States.Dashing")
		UE_DEFINE_GAMEPLAY_TAG(Invincible, "SoulTags.States.Invincible")
	}
	
	namespace Cooldown
	{
		UE_DEFINE_GAMEPLAY_TAG(Duration, "SoulTags.Cooldown.Duration")
		UE_DEFINE_GAMEPLAY_TAG(Skill_1_0, "SoulTags.Cooldown.Skill.1_0")
		UE_DEFINE_GAMEPLAY_TAG(Skill_1_1, "SoulTags.Cooldown.Skill.1_1")
		UE_DEFINE_GAMEPLAY_TAG(Skill_1_2, "SoulTags.Cooldown.Skill.1_2")
		UE_DEFINE_GAMEPLAY_TAG(Skill_2_0, "SoulTags.Cooldown.Skill.2_0")
		UE_DEFINE_GAMEPLAY_TAG(Skill_2_1, "SoulTags.Cooldown.Skill.2_1")
		UE_DEFINE_GAMEPLAY_TAG(Skill_2_2, "SoulTags.Cooldown.Skill.2_2")
		UE_DEFINE_GAMEPLAY_TAG(Skill_3_0, "SoulTags.Cooldown.Skill.3_0")
		UE_DEFINE_GAMEPLAY_TAG(Skill_3_1, "SoulTags.Cooldown.Skill.3_1")
		UE_DEFINE_GAMEPLAY_TAG(Skill_3_2, "SoulTags.Cooldown.Skill.3_2")
	}
	
	namespace InputTags
	{
		UE_DEFINE_GAMEPLAY_TAG(LMB, "SoulTags.InputTags.LMB")
		UE_DEFINE_GAMEPLAY_TAG(RMB, "SoulTags.InputTags.RMB")
		UE_DEFINE_GAMEPLAY_TAG(KEY_1, "SoulTags.InputTags.KEY_1")
		UE_DEFINE_GAMEPLAY_TAG(KEY_2, "SoulTags.InputTags.KEY_2")
		UE_DEFINE_GAMEPLAY_TAG(KEY_3, "SoulTags.InputTags.KEY_3")
		UE_DEFINE_GAMEPLAY_TAG(KEY_4, "SoulTags.InputTags.KEY_4")
		UE_DEFINE_GAMEPLAY_TAG(Dash, "SoulTags.InputTags.Dash")
	}
	
	namespace Events
	{
		namespace Montages
		{
			namespace Attacks
			{
				UE_DEFINE_GAMEPLAY_TAG(Melee, "SoulTags.Events.Montages.Attacks.Melee")
			}
			
			UE_DEFINE_GAMEPLAY_TAG(Fireball, "SoulTags.Events.Montages.Fireball")
		}
		
		namespace Combo
		{
			UE_DEFINE_GAMEPLAY_TAG(Open, "SoulTags.Events.Combo.Open")
			UE_DEFINE_GAMEPLAY_TAG(Close, "SoulTags.Events.Combo.Close")
			UE_DEFINE_GAMEPLAY_TAG(Branch, "SoulTags.Events.Combo.Branch")
		}
		
		namespace Melee
		{
			UE_DEFINE_GAMEPLAY_TAG(TraceBegin, "SoulTags.Events.Melee.TraceBegin")
			UE_DEFINE_GAMEPLAY_TAG(Hit, "SoulTags.Events.Melee.Hit")
		}
	}
}