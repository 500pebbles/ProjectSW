#pragma once

#include "CoreMinimal.h"
#include "GAS/GA/SoulMeleeAbility.h"
#include "SoulMobAttackAbility.generated.h"

class UAnimMontage;

/**
 * 런 몹 공격 (GA_MobAttack, 그래프 없음). USoulMobComponent가 고른 몽타주를 재생하고,
 * 몽타주의 USoulAnimNotify_MobHit 시각에 판정 구 안의 적에게 피해를 준다.
 * 피해 = CT_Damage_Mob(레벨) x 공격 규칙 배율 x 노티파이 배율 x BaseDamageScale x 층 배율.
 */
UCLASS()
class PROJECTSW_API USoulMobAttackAbility : public USoulMeleeAbility
{
	GENERATED_BODY()

public:
	USoulMobAttackAbility();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 팀 AM_MontageEvent처럼 노티파이 값이 없을 때 쓰는 판정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mob", meta = (Units = "cm"))
	float FallbackForwardOffset = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mob", meta = (Units = "cm"))
	float FallbackRadius = 0.f;

private:
	UFUNCTION()
	void HandleMontageEnded();

	/** 근접과 같은 피해 Spec (탄에 넘긴다) */
	FGameplayEffectSpecHandle MakeDamageSpec(float Multiplier) const;

	UFUNCTION()
	void HandleHitEvent(FGameplayEventData Payload);

	TObjectPtr<UAnimMontage> ActiveMontage;
	TWeakObjectPtr<AActor> ActiveTarget;
	bool bFirstHitDone = false;
};
