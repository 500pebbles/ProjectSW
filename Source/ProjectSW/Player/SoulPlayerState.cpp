
#include "SoulPlayerState.h"

#include "GAS/Core/SoulAbilitySystemComponent.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Net/UnrealNetwork.h"

ASoulPlayerState::ASoulPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<USoulAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	AttributeSet = CreateDefaultSubobject<USoulAttributeSet>("AttributeSet");
	
	NetUpdateFrequency = 60.f;
}

void ASoulPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ASoulPlayerState, Level);
}

UAbilitySystemComponent* ASoulPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAttributeSet* ASoulPlayerState::GetAttributeSet() const
{
	return AttributeSet;
}

void ASoulPlayerState::OnRep_Level(int32 OldLevel)
{
	
}
