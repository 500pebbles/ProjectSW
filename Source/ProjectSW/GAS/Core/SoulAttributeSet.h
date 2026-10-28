// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "SoulAttributeSet.generated.h"

/* Macro Accessors */
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName) 


USTRUCT()
struct FEffectProperties
{
	GENERATED_BODY()
	
	FEffectProperties()	{}
	
	FGameplayEffectContextHandle EffectContextHandle;
	
	UPROPERTY()
	UAbilitySystemComponent* SourceASC = nullptr;	
	UPROPERTY()
	AActor* SourceAvatarActor = nullptr;	
	UPROPERTY()
	AController* SourceController = nullptr;	
	UPROPERTY()
	ACharacter* SourceCharacter = nullptr;
	
	UPROPERTY()
	UAbilitySystemComponent* TargetASC = nullptr;	
	UPROPERTY()
	AActor* TargetAvatarActor = nullptr;	
	UPROPERTY()
	AController* TargetController = nullptr;	
	UPROPERTY()
	ACharacter* TargetCharacter = nullptr;	
};

UCLASS()
class PROJECTSW_API USoulAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	USoulAttributeSet();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;	
	
/* Attributes Section */	
public:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina)
	FGameplayAttributeData Stamina;	
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HealthRegeneration)
	FGameplayAttributeData HealthRegeneration;	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaRegeneration)
	FGameplayAttributeData StaminaRegeneration;	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackPower)
	FGameplayAttributeData AttackPower;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Defense)
	FGameplayAttributeData Defense;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalRate)
	FGameplayAttributeData CriticalRate;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalDamage)
	FGameplayAttributeData CriticalDamage;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CooldownReduction)
	FGameplayAttributeData CooldownReduction;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DamageAmplification)
	FGameplayAttributeData DamageAmplification;
	
	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData IncomingDamage;
	
	
/* Replication Callbacks */	
public:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth) const;
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldStamina) const;		
	
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const;	
	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina) const;
	UFUNCTION()
	void OnRep_HealthRegeneration(const FGameplayAttributeData& OldHealthRegeneration) const;	
	UFUNCTION()
	void OnRep_StaminaRegeneration(const FGameplayAttributeData& OldStaminaRegeneration) const;
	UFUNCTION()	
	void OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower) const;	
	UFUNCTION()
	void OnRep_Defense(const FGameplayAttributeData& OldDefense) const;	
	UFUNCTION()
	void OnRep_CriticalRate(const FGameplayAttributeData& OldCriticalRate) const;	
	UFUNCTION()
	void OnRep_CriticalDamage(const FGameplayAttributeData& OldCriticalDamage) const;	
	UFUNCTION()
	void OnRep_CooldownReduction(const FGameplayAttributeData& OldCooldownReduction) const;	
	UFUNCTION()
	void OnRep_DamageAmplification(const FGameplayAttributeData& OldDamageAmplification) const;		
	
/* Macro Accessors */
public:
	ATTRIBUTE_ACCESSORS(USoulAttributeSet, Health);
	ATTRIBUTE_ACCESSORS(USoulAttributeSet, Stamina);
	
	ATTRIBUTE_ACCESSORS(USoulAttributeSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(USoulAttributeSet, MaxStamina);	
	ATTRIBUTE_ACCESSORS(USoulAttributeSet, HealthRegeneration);
	ATTRIBUTE_ACCESSORS(USoulAttributeSet, StaminaRegeneration);	
	ATTRIBUTE_ACCESSORS(USoulAttributeSet, AttackPower);
	ATTRIBUTE_ACCESSORS(USoulAttributeSet, Defense);
	ATTRIBUTE_ACCESSORS(USoulAttributeSet, CriticalRate);
	ATTRIBUTE_ACCESSORS(USoulAttributeSet, CriticalDamage);
	ATTRIBUTE_ACCESSORS(USoulAttributeSet, CooldownReduction);
	ATTRIBUTE_ACCESSORS(USoulAttributeSet, DamageAmplification);	
	
	ATTRIBUTE_ACCESSORS(USoulAttributeSet, IncomingDamage);	
	
/* Cached Effect Properties */		
private:
	void SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props) const;
	
	
/* Damage Text UI Section */	
private:
	void ShowDamageText(const FEffectProperties& Props, float DamageAmount) const;
};
