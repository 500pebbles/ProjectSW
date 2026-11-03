#include "Inventory//Data/InventoryData.h"

#include "Item/ItemComponent.h"
#include "Item/Data/ItemData.h"

FInventoryDataEntry::FInventoryDataEntry()
{
	ID = FGuid::NewGuid();
}

FIntPoint FInventoryDataEntry::GetOccupiedSize() const
{
	if (!Item)
	{
		return FIntPoint::ZeroValue;
	}
	
	const FIntPoint DefaultGridSpace = Item->GetItemSize();
	return bIsRotated ? FIntPoint(DefaultGridSpace.Y, DefaultGridSpace.X) : DefaultGridSpace;
}

bool FInventoryDataEntry::IsOverlapping(FIntPoint InPosition, FIntPoint InSize) const
{
	const FIntPoint MySize = GetOccupiedSize();

	// 한쪽 사각형이 다른 쪽보다 완전히 왼쪽/오른쪽/위/아래에 있으면 안 겹침
	const bool bOverlapX = GridPosition.X < InPosition.X + InSize.X
						&& InPosition.X < GridPosition.X + MySize.X;
	const bool bOverlapY = GridPosition.Y < InPosition.Y + InSize.Y
						&& InPosition.Y < GridPosition.Y + MySize.Y;

	return bOverlapX && bOverlapY;
}

FIntPoint FInventoryDataArray::FindAvailablePosition(const UItemData* InItemData) const
{
	const FIntPoint Size = InItemData->GetItemSize();   // 회전 안 한 기본 크기
	if (Size.X <= 0 || Size.Y <= 0) return FIntPoint::NoneValue;

	// 좌상단 기준, 위에서 아래로 / 왼쪽에서 오른쪽으로 후보 위치를 훑는다
	for (int32 y = 0; y + Size.Y <= GridSize.Y; ++y)
	{
		for (int32 x = 0; x + Size.X <= GridSize.X; ++x)
		{
			const FIntPoint Candidate(x, y);

			bool bBlocked = false;
			for (const FInventoryDataEntry& Entry : ItemDataArray)
			{
				if (Entry.IsOverlapping(Candidate, Size))
				{
					bBlocked = true;
					break;
				}
			}

			if (!bBlocked)
			{
				return Candidate;
			}
		}
	}
	return FIntPoint::NoneValue;
}

FInventoryDataArray::FInventoryDataArray()
{
	OwnerComponent = nullptr;
}

FInventoryDataArray::FInventoryDataArray(UActorComponent* InOwnerComponent)
{
	OwnerComponent = InOwnerComponent;
}

void FInventoryDataArray::InitializeArray()
{
	ItemDataArray.Reset();
	ItemDataArray.Reserve(GridSize.X * GridSize.Y);
}

TArray<UItemData*> FInventoryDataArray::GetAllItems() const
{
	TArray<UItemData*> Items;
	Items.Reserve(ItemDataArray.Num());
	for (const auto ItemEntry : ItemDataArray)
	{
		if (IsValid(ItemEntry.Item))
		{
			Items.Add(ItemEntry.Item);
		}
	}
	
	return Items;
}

bool FInventoryDataArray::MoveItemPosition(const FGuid& InID, FIntPoint InNewPosition)
{
	// 아이템 존재하지 않으면 실패
	FInventoryDataEntry* CurrentItem = ItemDataArray.FindByPredicate([&InID](const FInventoryDataEntry& Item)
	{
		return InID == Item.GetID();
	});
	
	if (!CurrentItem)
	{
		return false;
	}
	
	if (!CheckSlotAvailable(InNewPosition, CurrentItem->GetOccupiedSize(), InID))
	{
		return false;
	}
	
	CurrentItem->GridPosition = InNewPosition;
	return true;
}

bool FInventoryDataArray::CheckSlotAvailable(FIntPoint InPosition, FIntPoint InSize, const FGuid& InIgnoreID) const
{
	if (InSize.X <= 0 || InSize.Y <= 0)
	{
		return false;
	}
	
	if (InPosition.X < 0 || InPosition.Y < 0
		|| InPosition.X + InSize.X > GridSize.X|| InPosition.Y + InSize.Y > GridSize.Y)
	{
		return false;
	}
	
	// 인벤토리 데이터(배열) 내의 다른 아이템들의 위치와 사이즈를 고려해서 넣을 수 있는지 판단
	for (const FInventoryDataEntry& Item : ItemDataArray)
	{
		if (Item.GetID() == InIgnoreID)
		{
			continue;
		}
		// 배열내의 아이템이 새로 넣을 아이템의 위치/사이즈를 고려해서 겹치면 넣을 수 없다고 판단.
		// 아이템 자체가 갖고 있는 위치/사이즈를 새로 넣을 아이템의 위치/사이즈와 비교
		if (Item.IsOverlapping(InPosition, InSize))
		{
			return false;
		}
	}
		
	return true;
}

const FInventoryDataEntry* FInventoryDataArray::FindItemWithID(const FGuid& InID) const
{
	// 펑터 람다 함수 -> 스코프로 InID를 받아서 해당 InID가 전달받은 아이템 엔트리 래퍼의 아이디와 같으면
	// 해당 아이템을 반환 -> Operator오버로딩으로 뺄 수 있지만 번거로움
	return ItemDataArray.FindByPredicate([&InID](const FInventoryDataEntry& Item)
	{
		return InID == Item.GetID();
	});
}

void FInventoryDataArray::AddItemData(const UItemComponent* InItemComponent)
{
	if (IsValid(InItemComponent))
	{
		AddItemData(InItemComponent->GetItemData());
	}
	return;
}

void FInventoryDataArray::AddItemData(UItemData* InItemData)
{
	if (IsValid(InItemData))
	{
		// 인벤토리에서 아이템을 넣을 수 있는 빈 공간 찾기
		FIntPoint TargetGridPosition = FindAvailablePosition(InItemData);
		if (TargetGridPosition.X < 0 || TargetGridPosition.Y < 0)
		{
			return;
		}
		
		FInventoryDataEntry NewItemEntry;
		NewItemEntry.Item = InItemData;
		NewItemEntry.GridPosition = TargetGridPosition;
		
		ItemDataArray.Add(NewItemEntry);
		
		if (InItemData->GetItemType() == EItemType::Consumable)
		{
			ConsumableItemDataArray.Add(NewItemEntry);
		}
		UE_LOG(LogTemp, Log, TEXT("Item Collected"))
	}
}

FRemoveResult FInventoryDataArray::RemoveItemData(const FGuid& InID, int32 RemoveCount)
{
	FRemoveResult Result;
	if (RemoveCount <= 0)
	{
		return Result;
	}
	
	// 아이템 데이터 Entry가 갖고 있는 인벤토리 내에서 위치 정보를 가져오기 -> 후에 UI 상의 위치 정보와 동일하다는 점을 이용해서 제거하기 위함
	const int32 ItemIndex = ItemDataArray.IndexOfByPredicate([&InID](const FInventoryDataEntry& Item)
	{
		return Item.GetID() == InID;
	});
	
	if (ItemIndex == INDEX_NONE)
	{
		return Result;
	}
	
	FInventoryDataEntry& Entry = ItemDataArray[ItemIndex];
	Result.RemovedCount = FMath::Min(RemoveCount, Entry.GetQuantity());
	Entry.SetQuantity(Entry.GetQuantity() - Result.RemovedCount);
	Result.RemainingItemCount = Entry.GetQuantity();
	
	// 아이템 잔여 수량이 없으면 아이템을 Array에서 제거
	if (Result.RemainingItemCount <= 0)
	{
		ItemDataArray.RemoveAt(ItemIndex);
	}
	return Result;
}
