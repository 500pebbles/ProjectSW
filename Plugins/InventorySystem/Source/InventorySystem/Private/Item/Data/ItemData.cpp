#include "Item/Data/ItemData.h"

#include "Item/Effect/ItemEffect.h"
#include "Item/Effect/ItemUseDefinition.h"


void UItemData::Initialize(const FDataTableRowHandle& InRowHandle)
{
	ItemRowHandle = InRowHandle;
	
	// 장비 아이템은 최초 초기화 시 레벨 제한을 무작위로 결정
	const FItemDataTable* Row = GetRow();
	if (Row && Row->ItemType == EItemType::Equipment)
	{
		// 테이블 입력 실수(Min > Max, 0 이하)를 방어
		const int32 MinLevel = FMath::Max(1, Row->LevelLimitMin);
		const int32 MaxLevel = FMath::Max(MinLevel, Row->LevelLimitMax);
		
		LevelLimit = FMath::RandRange(MinLevel, MaxLevel);
	}
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
	for (UItemEffect* Effect : CachedUseDefinition->Effects)
	{
		if (Effect)
		{
			Effect->ActivateItemEffect(User, this);
		}
	}
	
	return true;
}

bool UItemData::RemoveEquippedItem(AActor* User)
{
	if (!User || !CachedUseDefinition)
	{
		return false;
	}
	
	// 아이템 효과 적용
	for (UItemEffect* Effect : CachedUseDefinition->Effects)
	{
		if (Effect)
		{
			Effect->DeactivateItemEffect(User, this);
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

float UItemData::ScaleEffectValue(float BaseValue) const
{
	if (LevelLimit <= 1)
	{
		return BaseValue;
	}
	
	const FItemDataTable* Row = GetRow();
	const float ScalePerLevel = Row ? Row->StatScalePerLevel : 0.0f;
	
	return BaseValue * (1.0f + static_cast<float>(LevelLimit - 1) * ScalePerLevel);
}

float UItemData::GetScaledEffectValue(int32 Index) const
{
	const FItemDataTable* Row = GetRow();
	if (!Row || !Row->DescriptionValues.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: DescriptionValues[%d] 없음"), *GetNameSafe(this), Index);
		return 0.0f;
	}
	return ScaleEffectValue(Row->DescriptionValues[Index]);
}

FText UItemData::FormatScaledText(const FText& Source, const FItemDataTable& Row) const
{
	if (Row.DescriptionValues.IsEmpty())
	{
		return Source;
	}

	FFormatOrderedArguments Args;
	for (const float BaseValue : Row.DescriptionValues)
	{
		Args.Add(FText::AsNumber(FMath::RoundToInt(ScaleEffectValue(BaseValue))));
	}
	return FText::Format(Source, Args);
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

FGameplayTag UItemData::GetEquipmentType() const
{
	const FItemDataTable* ItemData = GetRow();
	
	if (!ItemData)
	{
		return FGameplayTag::EmptyTag;
	}
	
	return ItemData->EquipmentType;
}

FText UItemData::GetItemName() const
{
	const FItemDataTable* ItemData = GetRow();
	
	if (!ItemData)
	{
		return FText::GetEmpty();
	}
	
	return ItemData->ItemName;
}

FText UItemData::GetItemDescription() const
{
	const FItemDataTable* ItemData = GetRow();
	
	if (!ItemData)
	{
		return FText::GetEmpty();
	}
	
	return FormatScaledText(ItemData->ItemDescription, *ItemData);
}

FText UItemData::GetItemEffectDescription() const
{
	const FItemDataTable* ItemData = GetRow();
	
	if (!ItemData)
	{
		return FText::GetEmpty();
	}
	
	return FormatScaledText(ItemData->ItemEffectDescription, *ItemData);
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
