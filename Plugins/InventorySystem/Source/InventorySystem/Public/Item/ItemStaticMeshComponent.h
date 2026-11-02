// 

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Interface/HighlightMaterialInterface.h"
#include "ItemStaticMeshComponent.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UItemStaticMeshComponent : public UStaticMeshComponent, public IHighlightMaterialInterface
{
	GENERATED_BODY()
	
public:
	UItemStaticMeshComponent();
	
public:
	virtual void OnItemTraced() override;
	
	virtual void OffItemTraced() override;
	
private:
	UPROPERTY(EditAnywhere, Category = Inventory)
	TObjectPtr<UMaterialInstance> HighlightOverlayMaterial;
};
