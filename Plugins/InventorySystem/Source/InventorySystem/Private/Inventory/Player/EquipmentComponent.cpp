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
	
	// 새로 장착할 장비의 게임플레이 태그를 이용해 이전에 착용하고 있던 장비를 가져옴
	FGameplayTag EquipmentTag = Item->GetEquipmentType();
	LastEquippedItem = GetEquippedItem(EquipmentTag);
	
	if (LastEquippedItem)
	{
		LastEquippedItem->RemoveEquippedItem(GetOwner());
	}
	
	// TMap은 기존 Key값을 가진 Value를 덮어쓰도록 함 -> 중복 키 없음
	EquippedItems.Add(EquipmentTag, Item);
	// 신규 장비의 스탯 적용
	Item->UseItem(GetOwner());
	
	// UI에 반영
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
	
	// 기존 장비가 있다면 기존 장비의 스탯 해제
	if (IsValid(LastEquipment))
	{
		LastEquipment->RemoveEquippedItem(GetOwner());
	}
	
	// UI 반영
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

