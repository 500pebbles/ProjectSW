// 


#include "Item/HighlightStaticMeshComponent.h"

UHighlightStaticMeshComponent::UHighlightStaticMeshComponent()
{
	static ConstructorHelpers::FObjectFinder<UMaterialInstance> OverlayMaterialRef(
		TEXT("/InventorySystem/Materials/MI_HighlightMaterialInstance.MI_HighlightMaterialInstance"));
	
	if (OverlayMaterialRef.Succeeded())
	{
		OverlayMaterial = OverlayMaterialRef.Object;
		UE_LOG(LogTemp, Log, TEXT("Overlay Material Loaded"));
	}
}

void UHighlightStaticMeshComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UHighlightStaticMeshComponent::OnItemTraced()
{
	SetOverlayMaterial(OverlayMaterial);
}

void UHighlightStaticMeshComponent::OffItemTraced()
{
	SetOverlayMaterial(nullptr);
}
