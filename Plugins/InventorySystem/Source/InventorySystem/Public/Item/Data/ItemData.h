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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta=(MultiLine = true))
	FText ItemDescription;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item",  meta=(MultiLine = true))
	FText ItemEffectDescription;
	
	// 설명 텍스트의 {0}, {1}... 에 들어갈 기준 수치 (레벨 보정 전 값, 효과 BP의 기준값과 같게 입력)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
		TArray<float> DescriptionValues;
	
	// 아이템 사이즈
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin="1"), Category = "Item")
	FIntPoint ItemSize = FIntPoint(1, 1);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin="1") ,Category = "Item")
	int32 MaxStackSize = 1;
	
	// ---- 장비 전용 (ItemType == Equipment 일 때만 사용) ----
	// 아이템이 처음 생성될 때 [Min, Max] 범위에서 무작위로 레벨 제한이 정해진다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "1"))
	int32 LevelLimitMin = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "1"))
	int32 LevelLimitMax = 1;
	
	// 레벨 제한 1당 효과 수치 증가 비율 (0.1 = 레벨 제한이 1 오를 때마다 +10%)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "0.0"))
	float StatScalePerLevel = 0.1f;
	
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
	
	// 장비 레벨 제한 (장비가 아니면 0 = 제한 없음)
	UFUNCTION(BlueprintPure, Category = "Item")
	int32 GetLevelLimit() const { return LevelLimit; }
	
	// 효과 수치를 레벨 제한에 맞게 보정한다. 장비가 아니거나 레벨 제한이 1 이하면 입력값 그대로 반환
	UFUNCTION(BlueprintPure, Category = "Item")
	float ScaleEffectValue(float BaseValue) const;
	
	UFUNCTION(BlueprintPure, Category = "Item")
	float GetScaledEffectValue(int32 Index) const;
	
private:
	FText FormatScaledText(const FText& Source, const FItemDataTable& Row) const;
	
	
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
	TObjectPtr<const UItemUseDefinition> CachedUseDefinition;
	
	UPROPERTY()
	int32 LevelLimit = 0;
};
