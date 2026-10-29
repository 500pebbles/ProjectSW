// 


#include "AttributeMenuWidgetController.h"

#include "GAS/Core/SoulAttributeSet.h"
#include "GAS/Data/SoulAttributeInfo.h"
#include "Utility/SoulGameplayTags.h"

void UAttributeMenuWidgetController::BindCallbacksToDependencies()
{
	/* AttributeWidgetController의 Construct 단계에서 이 위젯컨트롤러를 Set으로 등록. 
	 * AttributeWidgetController가 보유한 모든 TextValueRow 서브위젯들은 같은 주소를 공유함.
	 * HUD에 정의한 GetAttributeMenuWidgetController의 소유권을 HUD쪽에서 들고있고
	 * 라이브러리에 정의한 GetAttributeMeneWidgetController 자체가 HUD에서 받아오기 때문.
	 * TextValueRow 서브위젯은 후자를 통해 컨트롤러에 접근. */

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetMaxHealthAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
	{
		FSoulAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(SoulTags::Attributes::MaxHealth);
		Info.AttributeValue = Data.NewValue;
		AttributeInfoDelegate.Broadcast(Info);
	});
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetMaxStaminaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
	{
		FSoulAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(SoulTags::Attributes::MaxStamina);
		Info.AttributeValue = Data.NewValue;
		AttributeInfoDelegate.Broadcast(Info);
	});
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetAttackPowerAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
	{
		FSoulAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(SoulTags::Attributes::AttackPower);
		Info.AttributeValue = Data.NewValue;
		AttributeInfoDelegate.Broadcast(Info);
	});
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetDefenseAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
	{
		FSoulAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(SoulTags::Attributes::Defense);
		Info.AttributeValue = Data.NewValue;
		AttributeInfoDelegate.Broadcast(Info);
	});
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetCriticalRateAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
	{
		FSoulAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(SoulTags::Attributes::CriticalRate);
		Info.AttributeValue = Data.NewValue;
		AttributeInfoDelegate.Broadcast(Info);
	});
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetCriticalDamageAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
	{
		FSoulAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(SoulTags::Attributes::CriticalDamage);
		Info.AttributeValue = Data.NewValue;
		AttributeInfoDelegate.Broadcast(Info);
	});
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetCooldownReductionAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
	{
		FSoulAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(SoulTags::Attributes::CooldownReduction);
		Info.AttributeValue = Data.NewValue;
		AttributeInfoDelegate.Broadcast(Info);
	});
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetDamageAmplificationAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
	{
		FSoulAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(SoulTags::Attributes::DamageAmplification);
		Info.AttributeValue = Data.NewValue;
		AttributeInfoDelegate.Broadcast(Info);
	});
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetHealthRegenerationAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
	{
		FSoulAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(SoulTags::Attributes::HealthRegeneration);
		Info.AttributeValue = Data.NewValue;
		AttributeInfoDelegate.Broadcast(Info);
	});
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetStaminaRegenerationAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
	{
		FSoulAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(SoulTags::Attributes::StaminaRegeneration);
		Info.AttributeValue = Data.NewValue;
		AttributeInfoDelegate.Broadcast(Info);
	});
}

void UAttributeMenuWidgetController::BroadcastInitialValues()
{
	USoulAttributeSet* AS = CastChecked<USoulAttributeSet>(AttributeSet);
	
	BroadcastAttributeInfo(SoulTags::Attributes::MaxHealth, AS->GetMaxHealth());
	BroadcastAttributeInfo(SoulTags::Attributes::MaxStamina, AS->GetMaxStamina());
	BroadcastAttributeInfo(SoulTags::Attributes::AttackPower, AS->GetAttackPower());
	BroadcastAttributeInfo(SoulTags::Attributes::Defense, AS->GetDefense());
	BroadcastAttributeInfo(SoulTags::Attributes::CriticalRate, AS->GetCriticalRate());
	BroadcastAttributeInfo(SoulTags::Attributes::CriticalDamage, AS->GetCriticalDamage());
	BroadcastAttributeInfo(SoulTags::Attributes::CooldownReduction, AS->GetCooldownReduction());
	BroadcastAttributeInfo(SoulTags::Attributes::DamageAmplification, AS->GetDamageAmplification());
	BroadcastAttributeInfo(SoulTags::Attributes::HealthRegeneration, AS->GetHealthRegeneration());
	BroadcastAttributeInfo(SoulTags::Attributes::StaminaRegeneration, AS->GetStaminaRegeneration());
}

void UAttributeMenuWidgetController::BroadcastAttributeInfo(const FGameplayTag& Tag, float Value) const
{
	FSoulAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(Tag);
	Info.AttributeValue = Value;
	AttributeInfoDelegate.Broadcast(Info);
}