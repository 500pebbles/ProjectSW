// 

#pragma once

#include "CoreMinimal.h"
#include "SoulDamageAbility.h"
#include "SoulRangedAbility.generated.h"

class UGameplayEffect;
class ASoulProjectileBase;

UCLASS()
class PROJECTSW_API USoulRangedAbility : public USoulDamageAbility
{
	GENERATED_BODY()
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintCallable)
	void SpawnProjectile();
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<ASoulProjectileBase> ProjectileClass;
};