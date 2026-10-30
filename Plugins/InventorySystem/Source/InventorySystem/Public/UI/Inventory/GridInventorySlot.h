// 

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GridInventorySlot.generated.h"

class UImage;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UGridInventorySlot : public UUserWidget
{
	GENERATED_BODY()
	
public:
	FORCEINLINE void SetTileIndex(int32 InTileIndex) { TileIndex = InTileIndex; }
	FORCEINLINE int32 GetTileIndex() const { return TileIndex; }
	
private:
	int32 TileIndex;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> InventorySlotImage;
};
