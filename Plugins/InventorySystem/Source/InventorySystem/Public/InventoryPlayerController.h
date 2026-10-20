// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InventoryPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;

DECLARE_LOG_CATEGORY_EXTERN(LogInventorySystem, Log, All);

/**
 * This player controller is just for input test only
 */
UCLASS()
class INVENTORYSYSTEM_API AInventoryPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	
	virtual void SetupInputComponent() override;
	
public:
	void InventoryInteract();
	
private:
	// EditDefaultsOnly parameter requires to initialize this variable in BluePrintClass manually
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TArray<TObjectPtr<UInputMappingContext>> IMC_Inventory;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UInputAction> InventoryInteractAction;
};
