
#include "Character/SoulCharacterBase.h"

#include "AbilitySystemComponent.h"

ASoulCharacterBase::ASoulCharacterBase()
{
	Weapon = CreateDefaultSubobject<UStaticMeshComponent>("Weapon");
	Weapon->SetupAttachment(GetMesh(), FName("WeaponSocket"));
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASoulCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

UAbilitySystemComponent* ASoulCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAttributeSet* ASoulCharacterBase::GetAttributeSet() const
{
	return AttributeSet;
}

void ASoulCharacterBase::InitAbilityActorInfo()
{
	
}

void ASoulCharacterBase::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level) const
{
	check(GameplayEffectClass);
	
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();	
	ContextHandle.AddSourceObject(this);	
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(GameplayEffectClass, Level, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), GetAbilitySystemComponent());
}

void ASoulCharacterBase::InitializeDefaultAttributes() const
{	
	ApplyEffectToSelf(DefaultAttributes, 1);
	ApplyEffectToSelf(InitializeAttributes, 1);
}
