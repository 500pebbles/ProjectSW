// 

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SoulGameplayAbility.generated.h"

UCLASS()
class PROJECTSW_API USoulGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	FGameplayTag StartupInputTag;
};