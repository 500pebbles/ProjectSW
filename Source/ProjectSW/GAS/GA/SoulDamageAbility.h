// 

#pragma once

#include "CoreMinimal.h"
#include "SoulGameplayAbility.h"
#include "SoulDamageAbility.generated.h"


UCLASS()
class PROJECTSW_API USoulDamageAbility : public USoulGameplayAbility
{
	GENERATED_BODY()
	
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	TMap<FGameplayTag, FScalableFloat> DamageTypes;
};