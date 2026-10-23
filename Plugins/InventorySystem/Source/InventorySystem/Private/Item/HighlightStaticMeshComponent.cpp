// 


#include "Item/HighlightStaticMeshComponent.h"

UHighlightStaticMeshComponent::UHighlightStaticMeshComponent()
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

void UHighlightStaticMeshComponent::OnItemTraced()
{
	SetOverlayMaterial(HighlightOverlayMaterial);
}

void UHighlightStaticMeshComponent::OffItemTraced()
{
	SetOverlayMaterial(nullptr);
}
