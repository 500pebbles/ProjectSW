// 


#include "SoulAbilitySystemGlobals.h"
#include "SoulGameplayEffectTypes.h"

FGameplayEffectContext* USoulAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FSoulGameplayEffectContext();
}