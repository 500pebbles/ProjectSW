// 


#include "SoulGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "GAS/Core/SoulAbilitySystemComponent.h"

bool USoulGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
		return false;
	if (StaminaCost <= 0.f)
		return true;
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	return ASC && ASC->GetNumericAttribute(USoulAttributeSet::GetStaminaAttribute()) >= StaminaCost;
}

void USoulGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (StaminaCost > 0.f && ASC && HasAuthority(&ActivationInfo))
	{
		ASC->ApplyModToAttribute(USoulAttributeSet::GetStaminaAttribute(), EGameplayModOp::Additive, -StaminaCost);
	}
}

void USoulGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (bCommitCostOnActivate && !CommitAbilityCost(Handle, ActorInfo, ActivationInfo))
	{
		// 비용 부족: 쿨타임은 통과했는데 비용에서 막힌 경우만 알린다 (피드백은 런 연출이 받는다)
		if (USoulAbilitySystemComponent* SoulASC = Cast<USoulAbilitySystemComponent>(ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr))
		{
			SoulASC->NotifyCostDenied(this);
		}
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}
