// 


#include "Item/ItemComponent.h"

#include "Item/Data/ItemData.h"


// Sets default values for this component's properties
UItemComponent::UItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UItemComponent::BeginPlay()
{
	Super::BeginPlay();
	InitItemData();
}

void UItemComponent::InitItemData()
{
	if (ItemRow.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: ItemRow가 비어 있음"), *GetNameSafe(GetOwner()));
		return;
	}
	
	ItemData = NewObject<UItemData>(this);
	ItemData->Initialize(ItemRow);
}

