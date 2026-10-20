
#include "SoulPlayerState.h"

#include "GAS/SoulAbilitySystemComponent.h"
#include "GAS/SoulAttributeSet.h"

ASoulPlayerState::ASoulPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<USoulAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	AttributeSet = CreateDefaultSubobject<USoulAttributeSet>("AttributeSet");
	
	NetUpdateFrequency = 60.f;
}

UAbilitySystemComponent* ASoulPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAttributeSet* ASoulPlayerState::GetAttributeSet() const
{
	return AttributeSet;
}
