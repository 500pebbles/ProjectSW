// 


#include "Item/HighlightStaticMeshComponent.h"

UHighlightStaticMeshComponent::UHighlightStaticMeshComponent()
{
}

void UHighlightStaticMeshComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OverlayMaterial = Cast<UMaterial>(GetOverlayMaterial());
	
	if (nullptr == OverlayMaterial)
	{
		UE_LOG(LogTemp, Log, TEXT("OverlayMaterial is nullptr"));
	}
}

void UHighlightStaticMeshComponent::OnItemTraced()
{
	OverlayMaterial->TwoSided = true;
}

void UHighlightStaticMeshComponent::OffItemTraced()
{
	OverlayMaterial->TwoSided = false;
}
