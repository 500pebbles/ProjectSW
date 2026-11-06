// 

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTooltip.generated.h"

class UTextBlock;
class UItemData;
class UImage;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UItemTooltip : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	
	void InitItemTooltip(UItemData* InItemData);
	
	void UpdateTooltipPosition(const FGeometry& ItemUIGeometry);
	
public:
	FORCEINLINE bool GetIsSet() const {return bIsSet;}
	
private:
	bool bIsSet = false;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> IconImage;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_ItemName;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_ItemDescription;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_ItemEffect;
};
