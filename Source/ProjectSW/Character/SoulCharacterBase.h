
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Interface/CombatInterface.h"
#include "SoulCharacterBase.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;
class UAttributeSet;

UCLASS()
class PROJECTSW_API ASoulCharacterBase : public ACharacter, public IAbilitySystemInterface, public ICombatInterface
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
	
/* GAS Section */
protected:
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level) const;
	
/* Initializer Section */
protected:
	void InitializeDefaultAttributes() const;
	
protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Setup")
	TSubclassOf<UGameplayEffect> DefaultAttributes;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Setup")
	TSubclassOf<UGameplayEffect> InitializeAttributes;
		
/* Component Section */	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<UStaticMeshComponent> Weapon;
};