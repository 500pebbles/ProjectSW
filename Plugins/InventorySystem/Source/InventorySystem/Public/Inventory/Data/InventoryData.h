#pragma once

#include "CoreMinimal.h"
#include "InventoryData.generated.h"

class UItemComponent;
class UItemData;

// 아이템 제거 결과를 담을 내부 구조체 -> 후에 아이템 제거 후 월드 상에 아이템을 스폰한다거나 UI에 반영하는 등 결과를 알아야 할 부분이 많아서 구조체로 묶어서 정의
// 후에 아이템 제거에 따른 추가적인 결과 반환이 필요한 경우 여기에 정의
struct FRemoveResult
{
	// 제거된 아이템 스택 수
	int32 RemovedCount = 0;
	
	// 제거 후 남은 수량
	int32 RemainingItemCount = 0;
};

// 아이템 하나에 대한 데이터 구조체, 인벤토리 어느 칸에 있는지 배치 정보와 회전 여부를 저장
USTRUCT(BlueprintType)
struct FInventoryDataEntry
{
	GENERATED_BODY()
	
	FInventoryDataEntry();
	
public:
	FORCEINLINE FGuid GetID() const {return ID;}
	FORCEINLINE UItemData* GetItem() const { return Item; }
	FORCEINLINE FIntPoint GetGridPosition() const { return GridPosition; }
	FORCEINLINE bool IsRotated() const { return bIsRotated; }
	
	FORCEINLINE void SetQuantity(int32 InQuantity) { Quantity = FMath::Max(0, InQuantity); }
	FORCEINLINE int32 GetQuantity() const { return Quantity; }
	
	// 회전 여부를 반영한 실제 차지하는 그리도 공간 크기 계산
	FIntPoint GetOccupiedSize() const;
	
	// 아이템이 배열의 특정 인덱스에 들어갔을 때 겹치는 아이템이 있는지 검사
	bool IsOverlapping(FIntPoint InPosition, FIntPoint InSize) const;
	
private:
	// 아이템 데이터는 인벤토리 컴포넌트가 소유할 것이므로 매우 밀접하게 연관되어 있어서 friend 선언
	// 아이템 데이터를 인벤토리 컴포넌트와 그 컴포넌트가 소유할 InventoryDataArray만 쓸 수 있도록 함
	// 캡슐화
	friend struct FInventoryDataArray;
	friend class UInventoryComponent;
	
	UPROPERTY()
	FGuid ID;
	
	UPROPERTY()
	TObjectPtr<UItemData> Item = nullptr;
	
	// 아이템 공간을 차지할 위치의 기준값(왼쪽 위)
	UPROPERTY()
	FIntPoint GridPosition = FIntPoint::NoneValue;
	
	// 회전 여부
	UPROPERTY()
	bool bIsRotated = false;
	
	UPROPERTY()
	int32 Quantity = 1;
};

//--------------------------------------------------------------------------//

// 인벤토리의 아이템 리스트
USTRUCT(BlueprintType)
struct FInventoryDataArray
{
	GENERATED_BODY()
	
	FInventoryDataArray();
	FInventoryDataArray(UActorComponent* InOwnerComponent);
	
public:
	void InitializeArray();
	
	// 목록에 아이템 추가
	void AddItemData(const UItemComponent* InItemComponent);
	void AddItemData(UItemData* InItemData);
	
	// 장비 슬롯에 아이템 장착
	const FInventoryDataEntry* AddItemDataToPosition(UItemData* EquippedItem, FIntPoint InPosition);
	
	// 목록에서 아이템 고유 ID를 이용해 아이템 제거
	FRemoveResult RemoveItemData(const FGuid& InID, int32 RemoveCount = 1);
	
	FIntPoint FindAvailablePosition(const UItemData* InItemData, const FGuid& InIgnoreID) const;

	// 아이템 목록 가져오기
	TArray<UItemData*> GetAllItems() const;
	
private:
	// ID를 받아 ID에 해당하는 아이템 데이터를 받은 위치로 이동시키는 함수
	bool MoveItemPosition(const FGuid& InID, FIntPoint InNewPosition);
	
	// UI에서 이동 시 IsOverlapping 이용하여 해당 위치가 이동할 수 있는 위치인지 판단하는 함수
	// ID는 본인의 ID를 받아서 본인은 제외하고 검사하도록 함
	bool CheckSlotAvailable(FIntPoint InPosition, FIntPoint InSize, const FGuid& InIgnoreID) const;
	
	// ID를 받아서 인벤토리 내에 ID에 해당하는 아이템 엔트리 래퍼를 반환하는 함수
	// 읽기 전용 -> 나중에 세이브/로드 혹은 값 조회 시 값 변경 불가
	const FInventoryDataEntry* FindItemWithID(const FGuid& InID) const;
	
public:
	// 인벤토리 공간 크기
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = "1"))
	FIntPoint GridSize;
	
	// 인벤토리 데이터 섹션
private:
	friend class UInventoryComponent;
	
	// 아이템 데이터 리스트
	UPROPERTY()
	TArray<FInventoryDataEntry> ItemDataArray;
	
	// 인벤토리 컴포넌트가 해당 아이템 리스트를 갖도록 지정한다.
	UPROPERTY()
	TObjectPtr<UActorComponent> OwnerComponent;
};
