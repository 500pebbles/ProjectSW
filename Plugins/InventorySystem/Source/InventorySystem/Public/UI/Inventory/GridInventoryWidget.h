// 

#pragma once

#include "CoreMinimal.h"
#include "InventoryBase.h"
#include "GridInventoryWidget.generated.h"

DECLARE_DELEGATE(OnToggleInventoryUIDelegate)

/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UGridInventoryWidget : public UInventoryBase
{
	GENERATED_BODY()
	
public:
	OnToggleInventoryUIDelegate OnToggleInventoryUI;
	
public:
	UGridInventoryWidget();
};
