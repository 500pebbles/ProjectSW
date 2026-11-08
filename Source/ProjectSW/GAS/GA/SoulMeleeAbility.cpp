// 


#include "SoulMeleeAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Utility/SoulGameplayTags.h"
#include "Run/Equipment/SoulEquipmentComponent.h"

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
	// 런 성장: 공격력 비율과 피해 증폭(%)을 곱한다. 기준 0이면 공격력 배율을 끈다
	const float PowerScale = AttackPowerReference > 0.f ? AttackPower / AttackPowerReference : 1.f;
	const float AmpScale = 1.f + SourceASC->GetNumericAttribute(USoulAttributeSet::GetDamageAmplificationAttribute()) / 100.f;
	
	for (auto& Pair : DamageTypes)
	{
		const float ScaledDamage = Pair.Value.GetValueAtLevel(GetAbilityLevel());
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, Pair.Key, ScaledDamage * Multiplier * PowerScale * AmpScale);
	}	
	// 런 무기: 물리 x ElementRatio를 무기 속성으로 더한다 (컴포넌트가 없는 팀 폰은 그대로)
	if (const USoulEquipmentComponent* Equipment = AvatarActor->FindComponentByClass<USoulEquipmentComponent>())
		Equipment->AppendWeaponDamage(SpecHandle);
	
	
	// Step 3 : 대상에게 적용.
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}