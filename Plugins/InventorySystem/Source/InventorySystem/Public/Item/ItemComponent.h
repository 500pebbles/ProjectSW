// 

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
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
	FORCEINLINE UItemData* GetItemData() const { return ItemData; }

	// 데이터 테이블 관련 색션
public:
	UFUNCTION(BlueprintCallable, Category="Item")
	void InitItemData();
	
	// 드롭 등으로 기존 아이템 인스턴스를 그대로 이어받아 설정 (레벨 제한 유지)
	void SetItemData(UItemData* InItemData);
	
private:
	UPROPERTY(Transient)
	TObjectPtr<UItemData> ItemData;
	
	// 아이템 데이터 색션
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta = (RowType = "/Script/InventorySystem.ItemDataTable"))
	FDataTableRowHandle ItemRow;
};
