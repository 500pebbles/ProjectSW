// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Item/ItemActorBase.h"
#include "Item/Interface/InventoryInteractInterface.h"
#include "ShopItemActor.generated.h"

class UShopItemInfoWidget;

UCLASS()
class PROJECTSW_API AShopItemActor : public AItemActorBase, public IInventoryInteractInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AShopItemActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// 다른 경로로 액터가 사라지는 경우를 대비
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
public:
	// 상점의 아이템이 상점 아이템으로써 유요한지 검사 -> 아이템이 Equipment 혹은 Consumable이고 교환 재료가 Misc일 때
	static bool IsValidStock(const FDataTableRowHandle& ItemRow, const FDataTableRowHandle& CostItemRow, int32 CostCount);
	
	void SetupStock(const FDataTableRowHandle& InCostItemRow, int32 InCostCount);
	
	// 인터페이스 함수 구현
public:
	virtual void HandleInventoryInteract(UInventoryComponent* UserInvComponent) override;
	
	virtual void OnInventoryFocusBegin(UInventoryComponent* PlayerInvComponent, FKey InteractionKey) override;
	
	virtual void OnInventoryFocusEnd(UInventoryComponent* PlayerInvComponent) override;
	
private:
	// 상점에서 생성한 아이템이 제대로 설정되었는지 확인하기 위한 함수
	bool IsItemSetupCorrectly() const;
	
	// 현재 표시중인 아이템 UI를 화면에서 내림
	void HideInfoWidget();
	
private:
	UPROPERTY(EditDefaultsOnly, Category="Shop")
	TSubclassOf<UShopItemInfoWidget> InfoWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UShopItemInfoWidget> InfoWidget;
	
	// 교환에 필요한 재료 아이템
	UPROPERTY()
	FDataTableRowHandle CostItemRow;
	
	// 교환에 필요한 재료 개수
	UPROPERTY()
	int32 CostItemCount;
};
