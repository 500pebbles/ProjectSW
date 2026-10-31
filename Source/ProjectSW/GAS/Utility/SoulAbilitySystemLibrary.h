// 

#pragma once

#include "CoreMinimal.h"
#include "GAS/Data/SoulEnemyTypeInfo.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SoulAbilitySystemLibrary.generated.h"


struct FGameplayEffectContextHandle;
class UAbilitySystemComponent;
class UAttributeMenuWidgetController;
class UOverlayWidgetController;

UCLASS()
class PROJECTSW_API USoulAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "SoulAbilitySystemLibrary")
	static UOverlayWidgetController* GetOverlayWidgetController(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category = "SoulAbilitySystemLibrary")
	static UAttributeMenuWidgetController* GetAttributeMenuWidgetController(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "SoulAbilitySystemLibrary")
	static void EnemyInitializeDefaultAttributes(const UObject* WorldContextObject, EEnemyType EnemyType, float Level, UAbilitySystemComponent* ASC);
	
	UFUNCTION(BlueprintCallable, Category = "SoulAbilitySystemLibrary")
	static void EnemyGiveStartupAbilities(const UObject* WorldContextObject, UAbilitySystemComponent* ASC, EEnemyType EnemyType);

	UFUNCTION(BlueprintCallable, Category = "SoulAbilitySystemLibrary")
	static USoulEnemyTypeInfo* GetEnemyTypeInfo(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category = "SoulAbilitySystemLibrary")
	static bool IsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle);

	UFUNCTION(BlueprintCallable, Category = "SoulAbilitySystemLibrary")
	static void SetIsCriticalHit(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, bool bIsCriticalHit);
};