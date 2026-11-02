#include "Item/Data/ItemData.h"

#include "Item/Effect/ItemEffect.h"
#include "Item/Effect/ItemUseDefinition.h"


void UItemData::Initialize(const FDataTableRowHandle& InRowHandle)
{
	ItemRowHandle = InRowHandle;
}

bool UItemData::UseItem(AActor* User)
{
	if (!User)
	{
		return false;
	}
	
	// 캐시된 아이템 효과가 없다면 캐시 로드
	if (!CachedUseDefinition)
	{
		CachedUseDefinition = GetRow()->UseDefinition.LoadSynchronous();
	}
	
	// 로드 후에도 아이템 효과가 없다면 효과 발동 안함
	// 같은 아이템에 대해서도 효과가 없다면 LoadSynchronous를 진행하지만 효과가 없다면 비용이 거의 없음
	if (!CachedUseDefinition)
	{
		return false;
	}
	
	// 아이템 효과 적용
	for (const UItemEffect* Effect : CachedUseDefinition->Effects)
	{
		if (Effect)
		{
			Effect->ActivateItemEffect(User, this);
		}
	}
	
	return true;
}

bool UItemData::IsItemUsable() const
{
	return !GetRow()->UseDefinition.IsNull();
}

bool UItemData::IsConsumedOnUse() const
{
	return CachedUseDefinition && CachedUseDefinition->bIsConsumable;
}

const FItemDataTable* UItemData::GetRow() const
{
	return ItemRowHandle.GetRow<FItemDataTable>(TEXT("UITemData::GetItemData"));
}

EItemType UItemData::GetItemType() const
{
	const FItemDataTable* ItemData = GetRow();
	
	if (!ItemData)
	{
		return EItemType::None;
	}
	
	return ItemData->ItemType;
}

FIntPoint UItemData::GetItemSize() const
{
	const FItemDataTable* ItemData = GetRow();
	
	if (!ItemData)
	{
		return FIntPoint::ZeroValue;
	}
	
	return ItemData->ItemSize;
}

UTexture2D* UItemData::GetItemIcon() const
{
	const FItemDataTable* ItemData = GetRow();
	if (!ItemData || ItemData->ItemIcon.IsNull())
	{
		return nullptr;
	}
	
	return ItemData->ItemIcon.LoadSynchronous();
}
