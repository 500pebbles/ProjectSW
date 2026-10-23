
#include "SoulAbilitySystemComponent.h"

#include "GAS/GA/SoulGameplayAbility.h"
#include "Utility/SoulGameplayTags.h"


void USoulAbilitySystemComponent::OnAbilityActorInfoSet()
{
	/* InitAbilityActorInfo로 인해 ASC가 등록되었을 경우 호출되는 Hook. 
	 * 모든 종류의 GameplayEffect 적용에 반응하는 콜백함수를 바인딩. 
	 * 현재 용도 : Widget에 바인딩하고 Effect적용시마다 UI Call 
	 * 최종적인 호출위치가 PossessedBy, 바인딩 자체는 서버가 보유한 ASC만 이루어짐. */
	
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &ThisClass::Client_EffectApplied);	
}

void USoulAbilitySystemComponent::GiveStartupAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
	/* Startup Ability는 기본적으로  Input GameplayTag와 함께 부여. */
	
	for (const auto AbilityClass : StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		if (const USoulGameplayAbility* SoulAbility = Cast<USoulGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.DynamicAbilityTags.AddTag(SoulAbility->StartupInputTag);			
			GiveAbility(AbilitySpec);
		}		
	}
}

void USoulAbilitySystemComponent::AbilityInputTagHeld(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag))
		{
			AbilitySpecInputPressed(AbilitySpec);  // Just set flag. 
			if (!AbilitySpec.IsActive()) TryActivateAbility(AbilitySpec.Handle); // Activate Ability.
		}
	}
}

void USoulAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;	
	
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag))
		{
			AbilitySpecInputReleased(AbilitySpec);  // Just set flag.
		}
	}
}

void USoulAbilitySystemComponent::Client_EffectApplied_Implementation(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	/* 종류를 불문하고 모든 GameplayEffect의 발생에 실행되는 콜백 함수. 
	 * 콜백의 파라미터로 GE와 ASC에 해당하는 대부분의 데이터에 접근.
	 * GameplayEffect -> ASC 적용은 서버에서만 이루어지고, 콜백 바인딩 자체도 서버에서만 진행 (PossessedBy).
	 * 따라서 Client RPC */
	
	// GameplayEffect가 보유한 태그를 전부 순회하고 원하는 작업을 수행. 현재는 보류상태. 
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);
	for (const FGameplayTag& Tag : TagContainer)
	{
		
	}
}