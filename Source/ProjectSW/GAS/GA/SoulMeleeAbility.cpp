// 


#include "SoulMeleeAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Utility/SoulGameplayTags.h"

void USoulMeleeAbility::ApplyMeleeDamage(AActor* TargetActor, float Multiplier)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority()) return;
	
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AvatarActor);
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	
	if (!SourceASC || !TargetASC || !DamageEffectClass) return;
	
	// Step 1 : 데미지 계산. 시전자 AttackPower × 배율.
	const float AttackPower = SourceASC->GetNumericAttribute(USoulAttributeSet::GetAttackPowerAttribute());
	
	// Step 2 : Spec 생성 + SetByCaller 주입. 팀 데미지 흐름(ExecCalc → IncomingDamage)이라 양수로 전달.
	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(AvatarActor);
	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), ContextHandle);
	if (!SpecHandle.IsValid()) return;
	
	//SpecHandle.Data->SetSetByCallerMagnitude(SoulTags::Combats::Damage, Damage);
	
	for (auto& Pair : DamageTypes)
	{
		const float ScaledDamage = Pair.Value.GetValueAtLevel(GetAbilityLevel());
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, Pair.Key, ScaledDamage * Multiplier);
	}	
	
	
	// Step 3 : 대상에게 적용.
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}