
#include "SoulAbilitySystemComponent.h"


void USoulAbilitySystemComponent::OnAbilityActorInfoSet()
{
	/* InitAbilityActorInfo로 인해 ASC가 등록되었을 경우 호출되는 Hook. 
	 * 모든 종류의 GameplayEffect 적용에 반응하는 콜백함수를 바인딩. 
	 * 현재 용도 : Widget에 바인딩하고 Effect적용시마다 UI Call */
	
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &ThisClass::EffectApplied);
}

void USoulAbilitySystemComponent::EffectApplied(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	/* 종류를 불문하고 모든 GameplayEffect의 발생에 실행되는 콜백 함수. 
	 * 콜백의 파라미터로 GE와 ASC에 해당하는 대부분의 데이터에 접근. */
	
	// GameplayEffect가 보유한 태그를 전부 순회하고, 원하는 작업을 수행. 현재는 보류상태. 
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);
	for (const FGameplayTag& Tag : TagContainer)
	{
		
	}
}