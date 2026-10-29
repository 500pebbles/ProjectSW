// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SoulInputConfig.generated.h"

class UInputAction;

USTRUCT(BlueprintType)
struct FSoulInputAction
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	const UInputAction* InputAction = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	FGameplayTag InputTag = FGameplayTag();
};

UCLASS()
class PROJECTSW_API USoulInputConfig : public UDataAsset
{
	GENERATED_BODY()
	
public:
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = false) const;
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	TArray<FSoulInputAction> AbilityInputActions;	
};
