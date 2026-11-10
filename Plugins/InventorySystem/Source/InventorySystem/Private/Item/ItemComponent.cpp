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
	if (ItemData)
	{
		return;
	}
	
	if (ItemRow.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: ItemRow가 비어 있음"), *GetNameSafe(GetOwner()));
		return;
	}
	
	ItemData = NewObject<UItemData>(this);
	ItemData->Initialize(ItemRow);
}

void UItemComponent::SetItemData(UItemData* InItemData)
{
	if (!IsValid(InItemData))
	{
		return;
	}
	
	ItemData = InItemData;
	ItemRow = InItemData->GetRowHandle();   // 이후 AItemActorBase::BeginPlay에서 메시/머티리얼 적용에 사용
}

