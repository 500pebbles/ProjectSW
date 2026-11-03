
#include "Character/SoulCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GAS/Core/SoulAbilitySystemComponent.h"
#include "Utility/SoulGameplayTags.h"
#include "Utility/SoulLog.h"

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
	ApplyEffectToSelf(ResistanceAttributes, 1);
}

void ASoulCharacterBase::GiveStartupAbilities()
{
	if (!HasAuthority()) return;
	
	USoulAbilitySystemComponent* SoulASC = Cast<USoulAbilitySystemComponent>(AbilitySystemComponent);
	SoulASC->GiveStartupAbilities(StartupAbilities);
	SoulASC->GiveSkillSlotChains(StartupSkillChains);
}

void ASoulCharacterBase::Die()
{
	// 사망 상태 태그. 근접 데미지 GE(Must Not Have Tags)가 이 태그를 보고 죽은 대상에게 다시 적용되지 않는다.
	if (AbilitySystemComponent) AbilitySystemComponent->AddLooseGameplayTag(SoulTags::States::Dead);
	SW_LOG("[%s] Died.", *GetName());
	
	Multicast_HandleDeath();
}

AActor* ASoulCharacterBase::GetAvatarActor_Implementation()
{
	return this;
}

bool ASoulCharacterBase::IsDead_Implementation() const
{
	return bDead;
}

void ASoulCharacterBase::Multicast_HandleDeath_Implementation()
{
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetEnableGravity(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	bDead = true;
}

FVector ASoulCharacterBase::GetCombatSocketLocation_Implementation()
{
	return Weapon->GetSocketLocation(CombatSocketName);
}

UAnimMontage* ASoulCharacterBase::GetHitReactMontage_Implementation()
{
	return HitReactMontage;
}

TArray<FTaggedMontage> ASoulCharacterBase::GetAttackMontages_Implementation()
{
	return AttackMontages;
}
