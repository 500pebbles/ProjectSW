#pragma once

#include "Engine/DataTable.h"
#include "ItemData.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	None = 0,   // 디폴트 (에러값)
	Equipment , // 장비
	Consumable, // 소비
	Misc,		// 기타
};

USTRUCT(BlueprintType)
struct FItemDataTable : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemType ItemType = EItemType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemDescription;
	
	// 아이템 사이즈
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin="1"))
	FIntPoint ItemSize = FIntPoint(1, 1);
};

UCLASS(BlueprintType, Blueprintable)
class UItemData : public UObject
{
	GENERATED_BODY()
	
public:
	// 이 아이템 인스턴스가 가리키는 행이 어떤 행인지 지정
	void Initialize(const FDataTableRowHandle& InRowHandle);
	
	// 데이터 테이블 행 조회
	const FItemDataTable* GetRow() const;
	
	FIntPoint GetItemSize() const;
	FName GetRowName() const {return ItemRowHandle.RowName;}
	
private:
	UPROPERTY()
	FDataTableRowHandle ItemRowHandle;
};
