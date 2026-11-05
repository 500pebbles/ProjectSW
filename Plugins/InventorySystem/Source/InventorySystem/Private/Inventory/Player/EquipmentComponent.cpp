// 


#include "Inventory/Player/EquipmentComponent.h"

#include "Item/Data/ItemData.h"

// Sets default values for this component's properties
UEquipmentComponent::UEquipmentComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

bool UEquipmentComponent::Equip(UItemData* Item, UItemData*& LastEquippedItem)
{
	LastEquippedItem = nullptr;
	
	if (!CanEquip(Item))
	{
		return false;
	}
	
	FGameplayTag EquipmentTag = Item->GetEquipmentType();
	LastEquippedItem = GetEquippedItem(EquipmentTag);
	
	// TMap은 기존 Key값을 가진 Value를 덮어쓰도록 함 -> 중복 키 없음
	EquippedItems.Add(EquipmentTag, Item);
	OnChangeEquipment.Broadcast(EquipmentTag, Item);
	return true;
}

UItemData* UEquipmentComponent::Unequip(const FGameplayTag& EquipmentTag)
{
	TObjectPtr<UItemData> LastEquipment;
	if (!EquippedItems.RemoveAndCopyValue(EquipmentTag, LastEquipment))
	{
		return nullptr;
	}
	
	OnChangeEquipment.Broadcast(EquipmentTag, nullptr);
	return LastEquipment;
}

bool UEquipmentComponent::CanEquip(const UItemData* Item)
{
	if (!IsValid(Item))
	{
		return false;
	}
	
	return Item->GetItemType() == EItemType::Equipment && Item->GetEquipmentType().IsValid();
}

UItemData* UEquipmentComponent::GetEquippedItem(const FGameplayTag& EquipmentTag)
{
	TObjectPtr<UItemData>* CurrentEquipment = EquippedItems.Find(EquipmentTag);
	if (!CurrentEquipment)
	{
		return nullptr;
	}
	
	return CurrentEquipment->Get();
}

