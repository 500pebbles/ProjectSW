// 

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HighlightMaterialInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UHighlightMaterialInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class INVENTORYSYSTEM_API IHighlightMaterialInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual void OnItemTraced() = 0;
	virtual void OffItemTraced() = 0;
};
