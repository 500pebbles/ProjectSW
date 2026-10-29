// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "SoulAbilitySystemGlobals.generated.h"

UCLASS()
class PROJECTSW_API USoulAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()
	
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
