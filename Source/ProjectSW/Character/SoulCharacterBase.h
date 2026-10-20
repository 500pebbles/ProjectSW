
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "SoulCharacterBase.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;

UCLASS()
class PROJECTSW_API ASoulCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	ASoulCharacterBase();
	
protected:
	virtual void BeginPlay() override;
	
/* GAS Section */	
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UAttributeSet* GetAttributeSet() const;
	
protected:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;
	
/* Component Section */	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<UStaticMeshComponent> Weapon;
	
};