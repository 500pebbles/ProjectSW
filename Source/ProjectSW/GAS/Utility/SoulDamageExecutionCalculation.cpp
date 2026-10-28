// 


#include "SoulDamageExecutionCalculation.h"
#include "AbilitySystemComponent.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Utility/SoulDefineMacro.h"
#include "Utility/SoulGameplayTags.h"

struct SoulDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalRate);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamage);
	
	SoulDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, Defense, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, CriticalRate, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, CriticalDamage, Source, false);
	}
};

static const SoulDamageStatics& DamageStatics()
{
	static SoulDamageStatics DStatics;
	return DStatics;
}

USoulDamageExecutionCalculation::USoulDamageExecutionCalculation()
{
	RelevantAttributesToCapture.Add(DamageStatics().DefenseDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalRateDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalDamageDef);
}

void USoulDamageExecutionCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// Setup Context. 
	const UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();	
	AActor* SourceAvatarActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetAvatarActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr;	

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;
	
	// 데미지 처리 Setup.
	float Damage = Spec.GetSetByCallerMagnitude(SoulTags::Combats::Damage);
	
	float TargetDefense = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DefenseDef, EvaluationParameters, TargetDefense);
	float SourceCriticalRate = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalRateDef, EvaluationParameters, SourceCriticalRate);
	float SourceCriticalDamage = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalDamageDef, EvaluationParameters, SourceCriticalDamage);

	// 최종데미지 계산.
	Damage = Damage * (100 / (100 + TargetDefense));
	Damage = FMath::Max<float>(Damage, 0.f);
	
	const float EffectiveCriticalRate = SourceCriticalRate;
	const bool bIsDamageCritical = FMath::RandRange(1, 100) < EffectiveCriticalRate;	
	if (bIsDamageCritical) Damage = Damage + (Damage * SourceCriticalDamage / 100);
		
	// 데미지 처리.
	const FGameplayModifierEvaluatedData EvaluatedData(USoulAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, Damage);
	OutExecutionOutput.AddOutputModifier(EvaluatedData);	
}