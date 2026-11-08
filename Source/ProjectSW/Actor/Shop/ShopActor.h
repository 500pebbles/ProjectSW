// 

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/Engine/DataTable.h"
#include "Runtime/Engine/Classes/GameFramework/Actor.h"
#include "ShopActor.generated.h"

class AShopItemActor;
// 가판대에 올릴 아이템을 감쌀 엔트리 -> 판매 아이템, 교환에 필요한 아이템, 요구 수량을 담음
USTRUCT(BlueprintType)
struct FShopStockEntry
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop", meta = (RowType = "/Script/InventorySystem.ItemDataTable"))
	FDataTableRowHandle ItemRow;
	
	UPROPERTY(EditAnywhere , BlueprintReadWrite, Category = "Shop", meta = (RowType = "/Script/InventorySystem.ItemDataTable"))
	FDataTableRowHandle CostItemRow;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop", meta = (ClampMin = "1"))
	int32 CostItemCount = 1;
};

UCLASS(Blueprintable)
class PROJECTSW_API AShopActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AShopActor();

	// constexpr : 컴파일 타임에 값이 정해지는 상수 -> UPROPERTY()로 슬롯 개수를 지정하도록 하면 BP로 액터를 생성할 때 미리 슬롯 컴포넌트를
	// 생성할 수 없음 -> BeginPlay에서 생성하기 위해서는 위치를 클래스 내에서 미리 계산해야함 => 번거로움
	static constexpr int32 ShopItemCount = 3;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	// 상품 1개를 슬롯에 생성하고 초기화하는 함수
	bool SpawnShopItem(int32 SlotIndex, const FShopStockEntry& Entry);
	
	bool BuildRandomEntry(FShopStockEntry& OutEntry) const;
	
	// 테이블에서 행 이름을 모으는 함수 -> 판매 가능한 아이템 행 이름만 분류해서 모음
	// bForCost가 true인 경우 Misc아이템
	void CollectSpecificRows(bool bForCost, TArray<FName>& OutRows) const;
	
private:
	UPROPERTY(VisibleAnywhere, Category="Shop")
	TObjectPtr<USceneComponent> SceneRoot;
	
	// 상품이 놓일 슬롯 위치 (BP로 조정)
	UPROPERTY(VisibleAnywhere, Category = "Shop")
	TArray<TObjectPtr<USceneComponent>> SlotPoints;
	
	// 판매 상품 목록
	UPROPERTY(EditAnywhere, Category = "Shop")
	TArray<FShopStockEntry> Stock;

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<AShopItemActor> ShopItemClass;
	
	UPROPERTY(EditAnywhere, Category = "Shop", meta=(RequiredAssetDataTags = "RowStructure=/Script/InventorySystem.ItemDataTable"))
	TObjectPtr<UDataTable> ItemTable;
	
	UPROPERTY(EditAnywhere, Category="Shop", meta=(ClampMin = "1"))
	int32 RandomCostCountMin = 1;
	
	UPROPERTY(EditAnywhere, Category="Shop", meta=(ClampMin = "1"))
	int32 RandomCostCountMax = 3;
};
