// 


#include "SoulDamageExecutionCalculation.h"
#include "AbilitySystemComponent.h"
#include "SoulAbilitySystemLibrary.h"
#include "SoulGameplayEffectTypes.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Utility/SoulDefineMacro.h"
#include "Utility/SoulGameplayTags.h"

struct SoulDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalRate);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamage);
	
	DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(SinResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(VirtueResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(GraceResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(HateResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(TormentResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(SolaceResistance);
	
	TMap<FGameplayTag, FGameplayEffectAttributeCaptureDefinition> GameplayTagsToCaptureDefs;
	
	SoulDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, CriticalRate, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, CriticalDamage, Source, false);
				
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, PhysicalResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, SinResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, VirtueResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, GraceResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, HateResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, TormentResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, SolaceResistance, Target, false)

		
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::CriticalRate, CriticalRateDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::CriticalDamage, CriticalRateDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::PhysicalResistance, PhysicalResistanceDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::SinResistance, SinResistanceDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::VirtueResistance, VirtueResistanceDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::GraceResistance, GraceResistanceDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::HateResistance, HateResistanceDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::TormentResistance, TormentResistanceDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::SolaceResistance, SolaceResistanceDef);
	}
};

static const SoulDamageStatics& DamageStatics()
{
	static SoulDamageStatics DStatics;
	return DStatics;
}

USoulDamageExecutionCalculation::USoulDamageExecutionCalculation()
{
	RelevantAttributesToCapture.Add(DamageStatics().CriticalRateDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalDamageDef);
	
	RelevantAttributesToCapture.Add(DamageStatics().PhysicalResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().SinResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().VirtueResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().GraceResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().HateResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().TormentResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().SolaceResistanceDef);
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
	
	// Calculation.
	float Damage = 0.f;
	for (const TTuple<FGameplayTag, FGameplayTag>& Pair : SoulTags::DamageTypes::GetDamageTypes())
	{		
		const FGameplayTag DamageTypeTag = Pair.Key;
		const FGameplayTag ResistanceTypeTag = Pair.Value;
		const FGameplayEffectAttributeCaptureDefinition CaptureDef = SoulDamageStatics().GameplayTagsToCaptureDefs[ResistanceTypeTag];
		
		float DamageTypeValue = Spec.GetSetByCallerMagnitude(Pair.Key, false, 0.f);
		
		float Resistance = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CaptureDef, EvaluationParameters, Resistance);
		Resistance = FMath::Clamp(Resistance, 0.f, 100.f);
		
		DamageTypeValue = DamageTypeValue * (100 / (100 + Resistance));	
		Damage += DamageTypeValue;
	}
	
	// Damage Modification Setup. 
	float SourceCriticalRate = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalRateDef, EvaluationParameters, SourceCriticalRate);
	float SourceCriticalDamage = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalDamageDef, EvaluationParameters, SourceCriticalDamage);

	
	const float EffectiveCriticalRate = SourceCriticalRate;
	const bool bIsDamageCritical = FMath::RandRange(1, 100) < EffectiveCriticalRate;	
	if (bIsDamageCritical) Damage = Damage + (Damage * SourceCriticalDamage / 100);		
	
	// Setup GE Context. 
	FGameplayEffectContextHandle EffectContextHandle = Spec.GetContext();
	USoulAbilitySystemLibrary::SetIsCriticalHit(EffectContextHandle, bIsDamageCritical);	
	
	// Execution.
	const FGameplayModifierEvaluatedData EvaluatedData(USoulAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, Damage);
	OutExecutionOutput.AddOutputModifier(EvaluatedData);	
}