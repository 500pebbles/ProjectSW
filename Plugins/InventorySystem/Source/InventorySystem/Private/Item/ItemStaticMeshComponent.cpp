// 


#include "Item/ItemStaticMeshComponent.h"

UItemStaticMeshComponent::UItemStaticMeshComponent()
{
	static ConstructorHelpers::FObjectFinder<UMaterialInstance> OverlayMaterialRef(
		TEXT("/InventorySystem/Materials/MI_HighlightMaterialInstance.MI_HighlightMaterialInstance"));
	
	if (OverlayMaterialRef.Succeeded())
	{
		HighlightOverlayMaterial = OverlayMaterialRef.Object;
		UE_LOG(LogTemp, Log, TEXT("Overlay Material Loaded"));
	}
	
	SetOverlayMaterial(nullptr);
}

void UItemStaticMeshComponent::OnItemTraced()
{
	SetOverlayMaterial(HighlightOverlayMaterial);
}

void UItemStaticMeshComponent::OffItemTraced()
{
	SetOverlayMaterial(nullptr);
}
