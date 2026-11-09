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

	// 발동 비용: 스태미나. 0이면 비용 없음 (Class Defaults > Costs)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Costs", meta = (ClampMin = "0"))
	float StaminaCost = 0.f;

	// 켜면 발동 즉시 비용을 낸다 (Commit 노드가 없는 GA_Dash용). 스킬은 CommitAbility가 내므로 끈다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Costs")
	bool bCommitCostOnActivate = false;

	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};