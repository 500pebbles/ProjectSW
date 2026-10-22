// 

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Interface/HighlightMaterialInterface.h"
#include "HighlightStaticMeshComponent.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API UHighlightStaticMeshComponent : public UStaticMeshComponent, public IHighlightMaterialInterface
{
	GENERATED_BODY()
	
public:
	UHighlightStaticMeshComponent();
	
	virtual void BeginPlay() override;
	
public:
	virtual void OnItemTraced() override;
	
	virtual void OffItemTraced() override;
	
private:
	TObjectPtr<class UMaterial> OverlayMaterial;
};
