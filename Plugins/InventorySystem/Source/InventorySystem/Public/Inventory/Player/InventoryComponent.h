// 

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/Data/InventoryData.h"
#include "InventoryComponent.generated.h"

class AItemActorBase;
class UItemData;
class UInventoryBase;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnInventoryItemAddedDelegate, const FInventoryDataEntry* /*AddedItem*/)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryItemRemovedDelegate, const FGuid, ItemInstanceID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemQuantityChangedDelegate, const FGuid, ItemInstanceID, int32, RemoveCount);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	FOnInventoryItemAddedDelegate OnInventoryItemAdded;
	
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemRemovedDelegate OnInventoryItemRemoved;
	
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemQuantityChangedDelegate OnItemQuantityChanged;
	
public:
	// Sets default values for this component's properties
	UInventoryComponent();

public:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	//UI
public:
	void CreateInventoryUIWidget();
	
	UFUNCTION(BlueprintCallable, Category = "Inventory" )
	void ToggleInventoryUI();
	
private:
	UPROPERTY()
	TObjectPtr<APlayerController> LocalGameController;
	
	// 아이템 습득 및 버리기
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CollectItem(AActor* TargetItem);
	
	// 아이템 사용 함수
	UFUNCTION(BlueprintCallable)
	void UseItem(const FGuid& ItemID);

	// 아이템 제거 함수, 내부적으로 수량을 통해 전체를 제거할지 수량을 조정할지 결정
	int32 RemoveItem(const FGuid& InItemID, int32 RemoveCount = 1);
	
	// 아이템 버리는 함수
	int32 DropItem(const FGuid& InID, int32 Count = 1);
	
	
private:
	// 아이템 버렸을 때 월드에 생성하는 함수
	bool SpawnDroppedItem(const FDataTableRowHandle& InItemRow);
	
	FTransform CalculateDropPosition(const AActor& OwnerActor) const;
	
private:
	UPROPERTY(EditAnywhere, Category = "Item")
	TSubclassOf<AItemActorBase> ItemActorClass;

	UPROPERTY(EditAnywhere, Category = "Item")
	float DropDistance = 100.f;
	
	// UI 섹션
private:
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInventoryBase> InventoryUIClass;

	UPROPERTY()
	TObjectPtr<UInventoryBase> InventoryUI;
	
	bool bIsInventoryUIOpen = false;
	
	// 인벤토리 데이터 섹션
public:
	FORCEINLINE const FInventoryDataArray& GetInventoryDataArray() const {return InventoryData;}
	
private:
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FInventoryDataArray InventoryData;
	
	// UI 조작을 위한 섹션
public:
	bool MoveItemPosition(const FGuid& TargetID, FIntPoint TargetPosition);
	bool CheckSlotAvailable(const FGuid& SlotID, FIntPoint TargetPosition) const;
	const FInventoryDataEntry* FindItemWithID(const FGuid& ItemID) const;
};
