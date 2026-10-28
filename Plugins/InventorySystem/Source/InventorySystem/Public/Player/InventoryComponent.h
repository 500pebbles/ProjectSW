// 

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/InventoryData.h"
#include "InventoryComponent.generated.h"

class UItemData;
class UInventoryBase;

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	// Sets default values for this component's properties
	UInventoryComponent();

public:
	// Called when the game starts
	virtual void BeginPlay() override;
	
public:
	void CreateInventoryUIWidget();
	
	UFUNCTION(BlueprintCallable, Category = "Inventory" )
	void ToggleInventoryUI();
	
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CollectItem(AActor* TargetItem);

private:
	UPROPERTY()
	TObjectPtr<APlayerController> LocalGameController;
	
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
};
