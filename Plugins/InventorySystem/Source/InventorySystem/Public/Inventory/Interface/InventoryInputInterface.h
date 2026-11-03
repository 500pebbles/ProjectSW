// 

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InventoryInputInterface.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToggleInventoryUIDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCollectItemDelegate, AActor*, TargetItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUseQuickSlotItemDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTraceItemChangedDelegate, bool, bHasTarget, FKey, InputKey, AActor*, TargetItemActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSwitchQuickSlotItemDelegate, bool, bIsHeadingNext);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSetupQuickSlotButtonTextDelegate, FKey, UseKey, FKey, SwitchPrevKey, FKey, SwitchNextKey);

// This class does not need to be modified.
UINTERFACE()
class UInventoryInputInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class INVENTORYSYSTEM_API IInventoryInputInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual FOnToggleInventoryUIDelegate& GetOnToggleInventoryUI() = 0;
	virtual FOnCollectItemDelegate& GetOnCollectItem() = 0;
	virtual FOnUseQuickSlotItemDelegate& GetOnUseQuickSlotItem() = 0;
	virtual FOnTraceItemChangedDelegate& GetOnTraceItemChanged() = 0;
	virtual FOnSwitchQuickSlotItemDelegate& GetSwitchQuickSlotItem() = 0;
	virtual FOnSetupQuickSlotButtonTextDelegate& GetSetupQuickSlotButtonText() = 0;
};
