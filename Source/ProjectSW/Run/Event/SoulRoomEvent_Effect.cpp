#include "Run/Event/SoulRoomEvent_Effect.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Utility/SoulLog.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

float ASoulRoomEvent_Effect::ComputeRatioDamage(const UAbilitySystemComponent* ASC, float Ratio, bool bCanKill)
{
	if (!ASC)
		return 0.f;
	const float MaxHealth = ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute());
	const float Health = ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute());
	const float Damage = MaxHealth * Ratio;
	// 기본은 비치명: 체력 1을 남긴다
	return bCanKill ? Damage : FMath::Max(0.f, FMath::Min(Damage, Health - 1.f));
}

void ASoulRoomEvent_Effect::ApplyMagnitudeEffect(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayEffect> Effect, FGameplayTag MagnitudeTag, float Value)
{
	if (!ASC || !Effect || Value <= 0.f)
		return;
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(ASC->GetAvatarActor());
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Effect, 1.f, Context);
	if (!Spec.IsValid())
		return;
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(Spec, MagnitudeTag, Value);
	// 자기 ASC에 건다: 데미지 숫자가 Source == Target에서 빠지고, 출처 없는 GE의 널 경로를 피한다
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void ASoulRoomEvent_Effect::ApplyEvent_Implementation(APawn* User)
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(User);
	if (!ASC)
		return;

	const float Before = ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute());
	const float Value = bIsDamage ? ComputeRatioDamage(ASC, Magnitude, bCanKill) : ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute()) * Magnitude;
	ApplyMagnitudeEffect(ASC, Effect, MagnitudeTag, Value);
	SW_LOG("[EVENT] %s hp %.0f -> %.0f (value %.0f)", bIsDamage ? TEXT("trap") : TEXT("heal"), Before, ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute()), Value);
}
