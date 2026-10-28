// 

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SoulCombatInterface.generated.h"

UINTERFACE(BlueprintType)
class USoulCombatInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTSW_API ISoulCombatInterface
{
	GENERATED_BODY()

public:
	virtual int32 GetPlayerLevel();
	
	virtual FVector GetCombatSocketLocation();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	UAnimMontage* GetHitReactMontage();
};
