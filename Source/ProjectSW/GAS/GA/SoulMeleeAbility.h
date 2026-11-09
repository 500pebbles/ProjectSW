// 

#pragma once

#include "CoreMinimal.h"
#include "SoulDamageAbility.h"
#include "SoulMeleeAbility.generated.h"


class UGameplayEffect;

UCLASS()
class PROJECTSW_API USoulMeleeAbility : public USoulDamageAbility
{
	GENERATED_BODY()

protected:
	// 대상에게 DamageEffectClass 적용. 데미지 = 시전자 AttackPower × Multiplier, SetByCaller(SoulTags.Combats.Damage)로 전달 → ExecCalc → IncomingDamage.
	UFUNCTION(BlueprintCallable)
	void ApplyMeleeDamage(AActor* TargetActor, float Multiplier = 1.f);

	// 레벨 성장: 피해 x AttackPower / AttackPowerReference. 0이면 끈다 (런 몹 GA는 0)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	float AttackPowerReference = 50.f;
};
