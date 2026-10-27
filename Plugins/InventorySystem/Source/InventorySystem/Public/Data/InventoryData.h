#pragma once

#include "CoreMinimal.h"
#include "InventoryData.generated.h"

class UItemComponent;
class UItemData;
// 아이템 하나에 대한 데이터 구조체
USTRUCT(BlueprintType)
struct FInventoryDataEntry
{
	GENERATED_BODY()
	
	FInventoryDataEntry();
	
private:
	// 아이템 데이터는 인벤토리 컴포넌트가 소유할 것이므로 매우 밀접하게 연관되어 있어서 friend 선언
	friend struct FInventoryDataArray;
	friend class UInventoryComponent;
	
	UPROPERTY()
	TObjectPtr<UItemData> Item = nullptr;
};

// 인벤토리의 아이템 리스트
USTRUCT(BlueprintType)
struct FInventoryDataArray
{
	GENERATED_BODY()
	
	FInventoryDataArray();
	FInventoryDataArray(UActorComponent* InOwnerComponent);
	
public:
	// 아이템 목록 가져오기
	TArray<UItemData*> GetAllItems() const;
	
	// 목록에 아이템 추가
	// 추가될 아이템의 포인터를 반환
	UItemData* AddItemData(UItemComponent* InItemComponent);
	UItemData* AddItemData(UItemData* InItemData);
	
	// 목록에서 아이템 포인터를 이용해 아이템 제거
	void RemoveItemData(UItemData* InItemData);
	
	
private:
	friend class UInventoryComponent;
	
	// 아이템 데이터 리스트
	UPROPERTY()
	TArray<FInventoryDataEntry> ItemDataArray;
	
	// 인벤토리 컴포넌트가 해당 아이템 리스트를 갖도록 지정한다.
	UPROPERTY()
	TObjectPtr<UActorComponent> OwnerComponent;
};