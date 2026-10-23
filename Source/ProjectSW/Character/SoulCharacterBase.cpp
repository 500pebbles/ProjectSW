
#include "Character/SoulCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GAS/Core/SoulAbilitySystemComponent.h"

ASoulCharacterBase::ASoulCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>("Weapon");
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

void ASoulCharacterBase::GiveStartupAbilities()
{
	if (!HasAuthority()) return;
	
	USoulAbilitySystemComponent* SoulASC = Cast<USoulAbilitySystemComponent>(AbilitySystemComponent);
	SoulASC->GiveStartupAbilities(StartupAbilities);
}

FVector ASoulCharacterBase::GetCombatSocketLocation()
{
	return Weapon->GetSocketLocation(CombatSocketName);
}
