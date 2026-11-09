#include "Run/Interact/SoulItemEffect_ApplyEffect.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Utility/SoulLog.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

void USoulItemEffect_ApplyEffect::ActivateItemEffect_Implementation(AActor* User, UItemData* Item)
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(User);
	if (!ASC || !Effect)
	{
		SW_LOG_WARNING("[ITEM] 효과 적용 불가: ASC=%d Effect=%d", ASC != nullptr, Effect != nullptr);
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(User);
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Effect, 1.f, Context);
	if (!Spec.IsValid())
		return;

	const float Magnitude = ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute()) * MaxHealthRatio + FlatMagnitude;
	if (MagnitudeTag.IsValid())
	{
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(Spec, MagnitudeTag, Magnitude);
	}
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	SW_LOG("[ITEM] %s used on %s magnitude=%.0f hp=%.0f", *Effect->GetName(), *User->GetName(), Magnitude, ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute()));
}
