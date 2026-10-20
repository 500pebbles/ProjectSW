#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace SoulTags
{
	namespace Attributes
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Health)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stamina)
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MaxHealth)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MaxStamina)		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HealthRegeneration)		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(StaminaRegeneration)				
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(AttackPower)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Defense)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CriticalRate)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CriticalDamage)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CooldownReduction)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(DamageAmplification)
	}
	
	namespace Abilities
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Primary)
	}
	
	namespace Effects
	{
		
	}
	
	namespace States
	{
		
		
	}
}