#include "Data/InventoryData.h"

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

void FInventoryDataArray::AddItemData(const UItemComponent* InItemComponent)
{
	if (IsValid(InItemComponent))
	{
		AddItemData(InItemComponent->GetItemData());
	}
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
	}
}

void FInventoryDataArray::RemoveItemData(FGuid InID)
{
	
}
