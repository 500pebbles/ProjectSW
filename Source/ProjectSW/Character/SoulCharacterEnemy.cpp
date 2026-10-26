
#include "SoulCharacterEnemy.h"

#include "Abilities/Tasks/AbilityTask.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GAS/Core/SoulAbilitySystemComponent.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "UI/Widget/SoulUserWidget.h"
#include "Utility/SoulDefineMacro.h"
#include "Utility/SoulLog.h"


ASoulCharacterEnemy::ASoulCharacterEnemy()
{
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
	
	AbilitySystemComponent = CreateDefaultSubobject<USoulAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	AttributeSet = CreateDefaultSubobject<USoulAttributeSet>("AttributeSet");
	
	HealthBar = CreateDefaultSubobject<UWidgetComponent>("HealthBar");
	HealthBar->SetupAttachment(GetRootComponent());
}

void ASoulCharacterEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	// ASC -> InitAbilityActorInfo.
	InitAbilityActorInfo();
	
	// Setup WidgetController. 
	USoulUserWidget* SoulUserWidget = Cast<USoulUserWidget>(HealthBar->GetUserWidgetObject());
	if (SoulUserWidget) SoulUserWidget->SetWidgetController(this);		
	
	// Bind Callbacks Attribute <-> WidgetComponent.
	const USoulAttributeSet* AS = CastChecked<USoulAttributeSet>(AttributeSet);
	if (!IsValid(AS)) return;	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data){ OnHealthChanged.Broadcast(Data.NewValue); });
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetMaxHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data){ OnMaxHealthChanged.Broadcast(Data.NewValue); });
	
	// Broadcast Initial Values.
	OnHealthChanged.Broadcast(AS->GetHealth());
	OnMaxHealthChanged.Broadcast(AS->GetMaxHealth());
}

void ASoulCharacterEnemy::InitAbilityActorInfo()
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	Cast<USoulAbilitySystemComponent>(AbilitySystemComponent)->OnAbilityActorInfoSet();	
	
	InitializeDefaultAttributes();
}

int32 ASoulCharacterEnemy::GetPlayerLevel()
{
	return Level;
}