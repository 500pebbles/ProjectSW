#pragma once

#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "ItemData.generated.h"

class UItemUseDefinition;
class UImage;

UENUM(BlueprintType)
enum class EItemType : uint8
{
	None = 0,   // 디폴트 (에러값)
	Equipment , // 장비
	Consumable, // 소비
	Misc,		// 기타
};

USTRUCT(BlueprintType)
struct INVENTORYSYSTEM_API FItemDataTable : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemType ItemType = EItemType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta=(Categories="Equipment.Slot"))
	FGameplayTag EquipmentType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemDescription;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemEffectDescription;
	
	// 아이템 사이즈
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin="1"), Category = "Item")
	FIntPoint ItemSize = FIntPoint(1, 1);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin="1") ,Category = "Item")
	int32 MaxStackSize = 1;
	
	// 아이템 효과 데이터 에셋
	UPROPERTY(EditAnywhere, Category="ItemEffects")
	TSoftObjectPtr<UItemUseDefinition> UseDefinition;
	
	// 아이템 이미지
	UPROPERTY(EditAnywhere, Category = "Item")
	TSoftObjectPtr<UTexture2D> ItemIcon;
	
	// 아이템 매시
	UPROPERTY(EditAnywhere, Category = "Item")
	TSoftObjectPtr<UStaticMesh> ItemMesh;
	
	// 아이템 마테리얼
	UPROPERTY(EditAnywhere, Category = "Item")
	TSoftObjectPtr<UMaterialInstance> ItemMaterialInstance;
};

UCLASS(BlueprintType, Blueprintable)
class INVENTORYSYSTEM_API UItemData : public UObject
{
	GENERATED_BODY()
	
public:
	// 이 아이템 인스턴스가 가리키는 행이 어떤 행인지 지정 (초기화 함수)
	void Initialize(const FDataTableRowHandle& InRowHandle);
	
	// 아이템 사용
	UFUNCTION(BlueprintCallable)
	bool UseItem(AActor* User);
	
	UFUNCTION(BlueprintCallable)
	bool RemoveEquippedItem(AActor* User);
	
	// 아이템이 사용 가능한 아이템인지 검사 (아이템 효과가 존재하는지 확인)
	bool IsItemUsable() const;
	
	// 아이템이 소모성 아이템인지 검사(아이템 효과가 존재하는 경우 아이템이 소모성인지 검사)
	bool IsConsumedOnUse() const;
	
	
public:
	// 데이터 테이블 행 조회
	const FItemDataTable* GetRow() const;
	
	FDataTableRowHandle GetRowHandle() const { return ItemRowHandle; }
	
	EItemType GetItemType() const;
	FGameplayTag GetEquipmentType() const;
	FText GetItemName() const;
	FText GetItemDescription() const;
	FText GetItemEffectDescription() const;
	FIntPoint GetItemSize() const;
	FName GetRowName() const {return ItemRowHandle.RowName;}
	
	UTexture2D* GetItemIcon() const;
	
private:
	UPROPERTY()
	FDataTableRowHandle ItemRowHandle;
	
	// 아이템 효과는 최초 1회만 로딩할 수 있도록 저장하기
	UPROPERTY(Transient)
	TSoftObjectPtr<const UItemUseDefinition> CachedUseDefinition;
};
