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
	DECLARE_ATTRIBUTE_CAPTUREDEF(FireResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(FrostResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(LightningResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(WindResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(HolyResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DarkResistance);
	
	TMap<FGameplayTag, FGameplayEffectAttributeCaptureDefinition> GameplayTagsToCaptureDefs;
	
	SoulDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, CriticalRate, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, CriticalDamage, Source, false);
				
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, PhysicalResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, FireResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, FrostResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, LightningResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, WindResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, HolyResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USoulAttributeSet, DarkResistance, Target, false)

		
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::CriticalRate, CriticalRateDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::CriticalDamage, CriticalRateDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::PhysicalResistance, PhysicalResistanceDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::FireResistance, FireResistanceDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::FrostResistance, FrostResistanceDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::LightningResistance, LightningResistanceDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::WindResistance, WindResistanceDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::HolyResistance, HolyResistanceDef);
		GameplayTagsToCaptureDefs.Add(SoulTags::Attributes::DarkResistance, DarkResistanceDef);
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
	RelevantAttributesToCapture.Add(DamageStatics().FireResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().FrostResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().LightningResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().WindResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().HolyResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().DarkResistanceDef);
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
	int32 DamageTypeCount = 0;
	int32 DamageIndex = 0;
	float Damage = 0.f;
	
	for (const TTuple<FGameplayTag, FGameplayTag>& Pair : SoulTags::DamageTypes::GetDamageTypes())
	{		
		const FGameplayTag DamageTypeTag = Pair.Key;
		const FGameplayTag ResistanceTypeTag = Pair.Value;
		const FGameplayEffectAttributeCaptureDefinition CaptureDef = SoulDamageStatics().GameplayTagsToCaptureDefs[ResistanceTypeTag];
		
		float DamageTypeValue = Spec.GetSetByCallerMagnitude(Pair.Key, false, 0.f);
		if (DamageTypeValue > KINDA_SMALL_NUMBER) DamageTypeCount = DamageIndex;
		
		float Resistance = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CaptureDef, EvaluationParameters, Resistance);
		Resistance = FMath::Clamp(Resistance, 0.f, 100.f);
		
		DamageTypeValue = DamageTypeValue * (100 / (100 + Resistance));	
		Damage += DamageTypeValue;
		DamageIndex++;
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
	USoulAbilitySystemLibrary::SetDamageType(EffectContextHandle, DamageTypeCount);
	
	// Execution.
	const FGameplayModifierEvaluatedData EvaluatedData(USoulAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, Damage);
	OutExecutionOutput.AddOutputModifier(EvaluatedData);	
}