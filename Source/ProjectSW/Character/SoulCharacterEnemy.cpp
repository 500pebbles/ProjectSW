
#include "SoulCharacterEnemy.h"

#include "Abilities/Tasks/AbilityTask.h"
#include "GAS/Core/SoulAbilitySystemComponent.h"
#include "GAS/Core/SoulAttributeSet.h"


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
	
	InitAbilityActorInfo();
}

void ASoulCharacterEnemy::InitAbilityActorInfo()
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	Cast<USoulAbilitySystemComponent>(AbilitySystemComponent)->OnAbilityActorInfoSet();
}

int32 ASoulCharacterEnemy::GetPlayerLevel()
{
	return Level;
}
