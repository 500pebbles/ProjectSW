
#include "SoulCharacterEnemy.h"

#include "Abilities/Tasks/AbilityTask.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/Core/SoulAbilitySystemComponent.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "GAS/Utility/SoulAbilitySystemLibrary.h"
#include "UI/Widget/SoulUserWidget.h"
#include "Utility/SoulDefineMacro.h"
#include "Utility/SoulGameplayTags.h"
#include "Utility/SoulLog.h"


ASoulCharacterEnemy::ASoulCharacterEnemy()
{
	// Component Setup. 
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
	
	AbilitySystemComponent = CreateDefaultSubobject<USoulAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	AttributeSet = CreateDefaultSubobject<USoulAttributeSet>("AttributeSet");
	
	HealthBar = CreateDefaultSubobject<UWidgetComponent>("HealthBar");
	HealthBar->SetupAttachment(GetRootComponent());
	
	// Movement Setup.
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
}

void ASoulCharacterEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	// ASC -> InitAbilityActorInfo.
	InitAbilityActorInfo();
	USoulAbilitySystemLibrary::EnemyGiveStartupAbilities(this, AbilitySystemComponent);
	
	// Setup WidgetController. 
	USoulUserWidget* SoulUserWidget = Cast<USoulUserWidget>(HealthBar->GetUserWidgetObject());
	if (SoulUserWidget) SoulUserWidget->SetWidgetController(this);		
	
	// Attribute 변경에 따른 감지 & 콜백 바인딩
	const USoulAttributeSet* AS = CastChecked<USoulAttributeSet>(AttributeSet);
	if (!IsValid(AS)) return;	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data){ OnHealthChanged.Broadcast(Data.NewValue); });
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetMaxHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data){ OnMaxHealthChanged.Broadcast(Data.NewValue); });
	
	// GameplayTag 변경에 따른 감지 & 콜백 바인딩. 
	AbilitySystemComponent->RegisterGameplayTagEvent(SoulTags::Combats::HitReact, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnHitReactTagChanged);
	
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

void ASoulCharacterEnemy::InitializeDefaultAttributes() const
{
	USoulAbilitySystemLibrary::EnemyInitializeDefaultAttributes(this, EnemyType, Level, AbilitySystemComponent);
}

int32 ASoulCharacterEnemy::GetPlayerLevel()
{
	return Level;
}

void ASoulCharacterEnemy::OnHitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	bHitReacting = NewCount > 0;
	
	GetCharacterMovement()->MaxWalkSpeed = bHitReacting ? 0.f : BaseWalkSpeed;
}
