#pragma once

#include "CoreMinimal.h"
#include "InventoryData.generated.h"

class UItemComponent;
class UItemData;

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
	// 추가될 아이템의 포인터를 반환
	void AddItemData(const UItemComponent* InItemComponent);
	void AddItemData(UItemData* InItemData);
	
	// 목록에서 아이템 고유 ID를 이용해 아이템 제거
	void RemoveItemData(FGuid InID);
	
	FIntPoint FindAvailablePosition(const UItemData* InItemData) const;

	// 아이템 목록 가져오기
	TArray<UItemData*> GetAllItems() const;
	
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