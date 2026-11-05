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
	
	
private:
	// 아이템 버렸을 때 월드에 생성하는 함수
	bool SpawnDroppedItem(const FDataTableRowHandle& InItemRow);
	
	FTransform CalculateDropPosition(const AActor& OwnerActor) const;
	
private:
	UPROPERTY(EditAnywhere, Category = "Item")
	TSubclassOf<AItemActorBase> ItemActorClass;

	UPROPERTY(EditAnywhere, Category = "Item")
	float DropDistance = 100.f;
	
	
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
	bool DropEquippedItem(const FGameplayTag& EquipemTag);
	
private:
	bool AddUnequipedItem(UItemData* Item, FIntPoint Position);
	
private:
	UPROPERTY()
	TObjectPtr<UEquipmentComponent> EquipmentComponent;
};
