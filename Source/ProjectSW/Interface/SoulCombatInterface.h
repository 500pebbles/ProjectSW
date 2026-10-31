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
	virtual int32 GetCharacterLevel();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FVector GetCombatSocketLocation();	
	
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	UAnimMontage* GetHitReactMontage();
	
	virtual void Die() = 0;
	
	
	UFUNCTION(BlueprintNativeEvent, BLueprintCallable)
	void FaceToTarget(const FVector& TargetLocation);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SetCombatTarget(AActor* CombatTargetActor);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	AActor* GetCombatTarget() const;
};
