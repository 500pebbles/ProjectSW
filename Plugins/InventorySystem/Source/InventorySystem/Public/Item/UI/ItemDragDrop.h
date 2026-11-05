// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/DragDropOperation.h"
#include "ItemDragDrop.generated.h"

class UItemData;

UENUM()
enum class EItemDragSource : uint8
{
	Inventory = 0,
	Equipment,
};

/**
 * 아이템 UI 위젯을 직접 이동하는게 아니라 동일한 크기 이미지를 별도로 HUD에 넣고 이를 이동시키도록 함으로써
 * 아이템 UI 위젯 본래의 로직과 드래그 전용 로직을 분리할 수 있음
 */
UCLASS()
class INVENTORYSYSTEM_API UItemDragDrop : public UDragDropOperation
{
	GENERATED_BODY()
	
public:
	// 드래그가 시작된 곳
	UPROPERTY()
	EItemDragSource DragSource = EItemDragSource::Inventory;
	
	// 아이템 엔트리의 ID, Source가 인벤토리 일때 사용가능
	UPROPERTY()
	FGuid ItemID;
	
	UPROPERTY()
	TObjectPtr<UItemData> ItemData;
	
	// 드래그 시작이 장비창일때
	UPROPERTY()
	FGameplayTag SourceEquipmentTag;
	
	// 아이템 위젯에서 마우스를 누른 지점
	UPROPERTY()
	FVector2D MousePressedPosition = FVector2D::ZeroVector;
};
