
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GAS/Core/SoulAbilitySystemComponent.h"
#include "Interface/SoulCombatInterface.h"
#include "SoulCharacterBase.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UAbilitySystemComponent;
class UAttributeSet;

DECLARE_MULTICAST_DELEGATE_OneParam(FSoulCharacterDiedSignature, AActor*);

UCLASS()
class PROJECTSW_API ASoulCharacterBase : public ACharacter, public IAbilitySystemInterface, public ISoulCombatInterface
{
	GENERATED_BODY()
	
public:
	ASoulCharacterBase();
	
protected:
	virtual void BeginPlay() override;
	
/* GAS Core Section */	
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UAttributeSet* GetAttributeSet() const;
	
protected:
	virtual void InitAbilityActorInfo();
	
protected:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;
	
/* Gameplay Effect Section */
protected:
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level) const;
	
/* Attribute Section */
protected:
	virtual void InitializeDefaultAttributes() const;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<UGameplayEffect> DefaultAttributes;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<UGameplayEffect> InitializeAttributes;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<UGameplayEffect> ResistanceAttributes;
	
/* Ability Section */
protected:
	void GiveStartupAbilities();
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	TArray<FSoulSkillSlotChain> StartupSkillChains;
	
	
/* Combat Interface Section */
public:
	virtual AActor* GetAvatarActor_Implementation() override;
	
	virtual void Die() override;
	
	virtual bool IsDead_Implementation() const override;
	
	UFUNCTION(NetMulticast, Reliable)
	virtual void Multicast_HandleDeath();
	
	virtual FVector GetCombatSocketLocation_Implementation() override;
	
	virtual UAnimMontage* GetHitReactMontage_Implementation() override;
	
	virtual TArray<FTaggedMontage> GetAttackMontages_Implementation() override;

	FSoulCharacterDiedSignature OnDied;
	
protected:
	bool bDead = false;
	
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TArray<FTaggedMontage> AttackMontages;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	FName CombatSocketName;
	
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<UAnimMontage> HitReactMontage;
	
/* Component Section */	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	TObjectPtr<USkeletalMeshComponent> Weapon;
};