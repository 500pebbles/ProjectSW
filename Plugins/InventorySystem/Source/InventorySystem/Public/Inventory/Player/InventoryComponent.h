// 

#pragma once

#include "CoreMinimal.h"
#include "EquipmentComponent.h"
#include "Components/ActorComponent.h"
#include "Inventory/Data/InventoryData.h"
#include "InventoryComponent.generated.h"

class UEquipmentComponent;
class UInventoryHUDWidget;
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
	virtual void InitializeComponent() override;
	
	// Called when the game starts
	virtual void BeginPlay() override;
	
	// 아이템 습득 및 버리기
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
		void CollectItem(AActor* TargetItem);
	
	UFUNCTION(BlueprintCallable)
		void UseQuickSlotItem();
	
	// 아이템 사용 함수
	UFUNCTION(BlueprintCallable)
		void UseItem(const FGuid& ItemID);

	// 아이템 제거 함수, 내부적으로 수량을 통해 전체를 제거할지 수량을 조정할지 결정
	int32 RemoveItem(const FGuid& InItemID, int32 RemoveCount = 1);
	
	// 아이템 버리는 함수
	int32 DropItem(const FGuid& InID, int32 Count = 1);
	
	// 인벤토리의 RowName에 해당하는 아이템 보유 개수 반환 -> 이후 UI에 보유 개수를 띄우기 위해 사용
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 CountItems(FName RowName) const;
	
	// 아이템을 제거한 후 아이템을 추가하는 변경이 가능한지 검사 -> 인벤토리에서 아이템이 제거되고 빈 공간이 생기는 것을 고려
	// 아이템 교환 혹은 제출 시 가능한지 UI등에 반영하기 위해 검사만 진행
	bool CanApplyChange(const TArray<FInventoryItemRequirement>& Remove, const TArray<UItemData*>& Add) const;
	
	// 실제 아이템 교환을 시도하는 함수 -> 제거 후 빈 공간이 생기는 것을 고려하여 아이템이 들어갈 수 있으면 true 반환
	bool TryApplyChange(const TArray<FInventoryItemRequirement>& Remove, const TArray<UItemData*>& Add);
	
private:
	// 아이템 버렸을 때 월드에 생성하는 함수
	bool SpawnDroppedItem(const FDataTableRowHandle& InItemRow);
	
	FTransform CalculateDropPosition(const AActor& OwnerActor) const;
	
	// CanApplyChange 와 TryApplyChange에서 제거 후 빈 공간에 들어갈 수 있는지 판정할 때 공통으로 사용할 함수
	// True인 경우 제거할 엔트리 ID와 추가할 아이템의 위치를 반환
	bool CheckChangeAvailable(const TArray<FInventoryItemRequirement>& Remove, const TArray<UItemData*>& Add,
					TArray<FGuid>& OutRemoveIDs, TArray<FIntPoint>& OutAddPositions) const;
	
private:
	UPROPERTY(EditAnywhere, Category = "Item")
	TSubclassOf<AItemActorBase> ItemActorClass;

	UPROPERTY(EditAnywhere, Category = "Item")
	float DropDistance = 100.f;
	
	// 현재 상호작용 가능한 아이템 액터를 저장해둘 변수 -> 실제 생명주기에는 영향을 미치지 않도록 WeakObjectPtr 이용
	TWeakObjectPtr<AActor> FocusedItem;
	
	
	//UI
public:
	// 인벤토리 HUD 생성
	void CreateInventoryHUDWidget();
	
	UFUNCTION()
	void HandleTraceItemChanged(bool bHasTarget, FKey InputKey, AActor* TargetItemActor);
	
	UFUNCTION()
	void SetupQuickSlotButtonText(FKey UseKey, FKey SwitchPrevKey, FKey SwitchNextKey);
	
	// 인벤토리 UI 생성
	void CreateInventoryUIWidget();
	
	UFUNCTION(BlueprintCallable, Category = "Inventory" )
	void ToggleInventoryUI();
	
	void CreateEquipmentUIWidget();
	
	UFUNCTION()
	void ToggleEquipmentUI();
	
private:
	void UpdateUIInputMode();
	
private:
	UPROPERTY()
	TObjectPtr<APlayerController> LocalGameController;
	
	// UI 섹션
private:
	UPROPERTY(EditAnywhere, Category = "Inventory UI")
	TSubclassOf<UInventoryHUDWidget> InventoryHUDWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UInventoryHUDWidget> InventoryHUDWidget;	
	
	UPROPERTY(EditAnywhere, Category = "Inventory UI")
	TSubclassOf<UInventoryBase> InventoryUIClass;

	UPROPERTY()
	TObjectPtr<UInventoryBase> InventoryUI;
	
	bool bIsInventoryUIOpen = false;
	
	UPROPERTY(EditAnywhere, Category = "Inventory UI")
	TSubclassOf<UUserWidget> EquipUIClass;
	
	UPROPERTY()
	TObjectPtr<UUserWidget> EquipUI;
	
	bool bIsEquipUIOpen = false;
	
	// 퀵슬롯 관련 섹션
private:
	// 인벤토리에서 소비 아이템의 종류를 가져오는 함수
	TArray<FName> GetConsumableItemKinds() const;
	
	const FInventoryDataEntry* FindFirstConsumableKind(FName Kind) const;
	
	void RefreshQuickSlot();
	
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CycleQuickSlot(bool bIsHeadingNext);
	
private:
	// 현재 선택된 아이템의 종류
 	FName SeletedQuickSlotItemKind = NAME_None;
	
	// 텍스트 할당을 위해 저장하는 캐시값 -> Beginplay 호출 순서 문제 해결을 위함
	FKey CachedUseKey;
	FKey CachedNextKey;
	FKey CachedPrevKey;
	
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
	// 장비칸에서 드래그해온 아이템은 ItemEntry로 감싸져있지 않기 때문에 FGuid 초기화를 위해 함수 정의
	bool CheckSlotAvailableEquipment(const UItemData* Item, FIntPoint TargetPosition) const;
	const FInventoryDataEntry* FindItemWithID(const FGuid& ItemID) const;
	
	// 장비 착용 관련 섹션
public:
	FORCEINLINE UEquipmentComponent* GetEquippedItemComponent() const {return EquipmentComponent;}
	
	bool EquipItem(const FGuid& ItemID);
	
	bool UnequipItem(const FGameplayTag& EquipmentTag, FIntPoint Position);
	
	// 장비 교체 시 인벤토리에 공간 없으면 바닥에 버리도록 하는 함수
	bool DropEquippedItem(const FGameplayTag& EquipmentTag);
	
private:
	bool AddUnequipedItem(UItemData* Item, FIntPoint Position);
	
private:
	UPROPERTY()
	TObjectPtr<UEquipmentComponent> EquipmentComponent;
};
