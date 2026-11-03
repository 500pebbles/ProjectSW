// 

#pragma once

#include "CoreMinimal.h"
#include "SoulGameplayAbility.h"
#include "Interface/SoulCombatInterface.h"
#include "SoulDamageAbility.generated.h"


UCLASS()
class PROJECTSW_API USoulDamageAbility : public USoulGameplayAbility
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void ApplyDamage(AActor* TargetActor);
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	TMap<FGameplayTag, FScalableFloat> DamageTypes;
	
	UFUNCTION(BlueprintPure)
	FTaggedMontage GetRandomTaggedMontage(const TArray<FTaggedMontage>& TaggedMontages);
};