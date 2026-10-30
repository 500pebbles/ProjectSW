// 

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemComponent.generated.h"


class UItemData;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UItemComponent();

public:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	FORCEINLINE UItemData* GetItemData() const { return ItemData; }

	// 데이터 테이블 관련 색션
public:
	UFUNCTION(BlueprintCallable, Category="Item")
	void InitItemData();
	
	
private:
	UPROPERTY(Transient)
	TObjectPtr<UItemData> ItemData;
	
	// 아이템 데이터 색션
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta = (RowType = "/Script/InventorySystem.ItemDataTable"))
	FDataTableRowHandle ItemRow;
};
