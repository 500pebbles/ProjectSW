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
		UE_DEFINE_GAMEPLAY_TAG(FireResistance, "SoulTags.Attributes.FireResistance")
		UE_DEFINE_GAMEPLAY_TAG(FrostResistance, "SoulTags.Attributes.FrostResistance")
		UE_DEFINE_GAMEPLAY_TAG(LightningResistance, "SoulTags.Attributes.LightningResistance")
		UE_DEFINE_GAMEPLAY_TAG(WindResistance, "SoulTags.Attributes.WindResistance")
		UE_DEFINE_GAMEPLAY_TAG(HolyResistance, "SoulTags.Attributes.HolyResistance")
		UE_DEFINE_GAMEPLAY_TAG(DarkResistance, "SoulTags.Attributes.DarkResistance")
	}
	
	namespace Combats
	{
		UE_DEFINE_GAMEPLAY_TAG(Damage, "SoulTags.Combats.Damage")
		UE_DEFINE_GAMEPLAY_TAG(HitReact, "SoulTags.Combats.HitReact")
	}
	
	namespace DamageTypes
	{
		UE_DEFINE_GAMEPLAY_TAG(Physical, "SoulTags.DamageTypes.Physical")
		UE_DEFINE_GAMEPLAY_TAG(Fire, "SoulTags.DamageTypes.Fire")
		UE_DEFINE_GAMEPLAY_TAG(Frost, "SoulTags.DamageTypes.Frost")
		UE_DEFINE_GAMEPLAY_TAG(Lightning, "SoulTags.DamageTypes.Lightning")
		UE_DEFINE_GAMEPLAY_TAG(Wind, "SoulTags.DamageTypes.Wind")
		UE_DEFINE_GAMEPLAY_TAG(Holy, "SoulTags.DamageTypes.Holy")
		UE_DEFINE_GAMEPLAY_TAG(Dark, "SoulTags.DamageTypes.Dark")
		
		const TMap<FGameplayTag, FGameplayTag>& GetDamageTypes()
		{
			static const TMap<FGameplayTag, FGameplayTag> AllDamageTypes = 
			{
				{ Physical, Attributes::PhysicalResistance}, 
				{ Fire, Attributes::FireResistance},
				{ Frost, Attributes::FrostResistance},
				{ Lightning, Attributes::LightningResistance},
				{ Wind, Attributes::WindResistance},
				{ Holy, Attributes::HolyResistance},
				{ Dark, Attributes::DarkResistance}
			};			
			
			return AllDamageTypes;
		}
	}
	
	namespace Abilities
	{
		UE_DEFINE_GAMEPLAY_TAG(Primary, "SoulTags.Abilities.Primary")
		UE_DEFINE_GAMEPLAY_TAG(Attack, "SoulTags.Abilities.Attack")
		UE_DEFINE_GAMEPLAY_TAG(Dash, "SoulTags.Abilities.Dash")
	} 
	
	namespace States
	{
		UE_DEFINE_GAMEPLAY_TAG(Dead, "SoulTags.States.Dead")
		UE_DEFINE_GAMEPLAY_TAG(Attacking, "SoulTags.States.Attacking")
		UE_DEFINE_GAMEPLAY_TAG(Dashing, "SoulTags.States.Dashing")
		UE_DEFINE_GAMEPLAY_TAG(Invincible, "SoulTags.States.Invincible")
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