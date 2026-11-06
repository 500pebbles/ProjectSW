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
	void ShowInteractionMessage(FKey InputKey, const FText& ItemName);
	
	UFUNCTION(BlueprintImplementableEvent)
	void HideInteractionMessage();
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetQuickSlotIcon(UTexture2D* Icon);
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetQuickSlotButtonText(FKey UseKey, FKey SwitchPrevKey, FKey SwitchNextKey);
};
