// 

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InventoryInteractInterface.generated.h"

class UInventoryComponent;
// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInventoryInteractInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class INVENTORYSYSTEM_API IInventoryInteractInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual void HandleInventoryInteract(UInventoryComponent* UserInvComponent) = 0;
	
	// 플레이어가 아이템을 바라볼 때 호출되는 함수
	virtual void OnInventoryFocusBegin(UInventoryComponent* PlayerInvComponent, FKey InteractionKey) {};
	
	virtual void OnInventoryFocusEnd(UInventoryComponent* PlayerInvComponent) {};
};
