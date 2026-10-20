
#include "SoulCharacterEnemy.h"

#include "GAS/SoulAbilitySystemComponent.h"
#include "GAS/SoulAttributeSet.h"


ASoulCharacterEnemy::ASoulCharacterEnemy()
{
	AbilitySystemComponent = CreateDefaultSubobject<USoulAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	AttributeSet = CreateDefaultSubobject<USoulAttributeSet>("AttributeSet");
}

void ASoulCharacterEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}
