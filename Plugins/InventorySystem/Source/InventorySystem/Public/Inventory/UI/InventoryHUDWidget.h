// 

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryHUDWidget.generated.h"


class UTexture2D;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UInventoryHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent)
	void ShowInteractionMessage();
	
	UFUNCTION(BlueprintImplementableEvent)
	void HideInteractionMessage();
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetQuickSlotIcon(UTexture2D* Icon);
};
