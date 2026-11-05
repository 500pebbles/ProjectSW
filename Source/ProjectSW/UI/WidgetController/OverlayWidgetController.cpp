// 


#include "OverlayWidgetController.h"
#include "GAS/Core/SoulAttributeSet.h"

void UOverlayWidgetController::BroadcastInitialValues()
{	
	/* UI와 AttributeSet이 모두 보유되었을 때 최초값을 수동으로 Broadcast.
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
	/* ASC에서 AttributeData의 변경값을 감지하고 콜백함수 바인딩. 콜백함수는 BlueprintAssignable로 BP에 정의한 이벤트 실행.
	 * 바인딩 시점은 AttributeSet과 OverlayWidgetController가 모두 준비되는 GetOverlayWidgetController. */
	
	// Attributes UI.
	const USoulAttributeSet* SoulAttributeSet = Cast<USoulAttributeSet>(AttributeSet);
	if (!IsValid(SoulAttributeSet)) return;
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SoulAttributeSet->GetHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data) { OnHealthChanged.Broadcast(Data.NewValue);});
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SoulAttributeSet->GetMaxHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data) { OnMaxHealthChanged.Broadcast(Data.NewValue);});
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SoulAttributeSet->GetStaminaAttribute()).AddLambda([this](const FOnAttributeChangeData& Data) { OnStaminaChanged.Broadcast(Data.NewValue);});
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SoulAttributeSet->GetMaxStaminaAttribute()).AddLambda([this](const FOnAttributeChangeData& Data) { OnMaxStaminaChanged.Broadcast(Data.NewValue);});
}