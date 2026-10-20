// 


#include "OverlayWidgetController.h"

#include "GAS/SoulAttributeSet.h"
#include "Utility/SoulDefineMacro.h"

void UOverlayWidgetController::BroadcastInitialValues()
{	
	/* UI와 AttributeSet이 모두 보유되었을 때 최초값을 Broadcast.
	 * 호출 시점은 CharacterPlayer의 InitAbilityActorInfo (= InitOverlay) */	
	
	const USoulAttributeSet* SoulAttributeSet = Cast<USoulAttributeSet>(AttributeSet);
	if (!IsValid(SoulAttributeSet)) return;
	
	OnMaxHealthChanged.Broadcast(SoulAttributeSet->GetMaxHealth());
	OnHealthChanged.Broadcast(SoulAttributeSet->GetHealth());
	OnMaxStaminaChanged.Broadcast(SoulAttributeSet->GetMaxStamina());
	OnStaminaChanged.Broadcast(SoulAttributeSet->GetStamina());	
}

void UOverlayWidgetController::BindCallbacksToDependencies()
{
	/* ASC에서 AttributeData의 변경값을 감지하고 콜백함수 바인딩. 콜백함수는 BlueprintAssignable로 BP 정의.
	 * 바인딩 시점은 AttributeSet과 OverlayWidgetController가 모두 준비되는 GetOverlayWidgetController. */
	
	const USoulAttributeSet* SoulAttributeSet = Cast<USoulAttributeSet>(AttributeSet);
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SoulAttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::HealthChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SoulAttributeSet->GetMaxHealthAttribute()).AddUObject(this, &ThisClass::MaxHealthChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SoulAttributeSet->GetStaminaAttribute()).AddUObject(this, &ThisClass::StaminaChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SoulAttributeSet->GetMaxStaminaAttribute()).AddUObject(this, &ThisClass::MaxStaminaChanged);
}

void UOverlayWidgetController::HealthChanged(const FOnAttributeChangeData& Data) const
{
	OnHealthChanged.Broadcast(Data.NewValue);
}

void UOverlayWidgetController::MaxHealthChanged(const FOnAttributeChangeData& Data) const
{
	OnMaxHealthChanged.Broadcast(Data.NewValue);
}

void UOverlayWidgetController::StaminaChanged(const FOnAttributeChangeData& Data) const
{
	OnStaminaChanged.Broadcast(Data.NewValue);
}

void UOverlayWidgetController::MaxStaminaChanged(const FOnAttributeChangeData& Data) const
{
	OnMaxStaminaChanged.Broadcast(Data.NewValue);
}
