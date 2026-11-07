// 


#include "ShopItemActor.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/Data/InventoryData.h"
#include "Inventory/Player/InventoryComponent.h"
#include "Item/ItemComponent.h"
#include "Item/Data/ItemData.h"
#include "UI/Widget/Shop/ShopItemInfoWidget.h"


// Sets default values
AShopItemActor::AShopItemActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AShopItemActor::BeginPlay()
{
	Super::BeginPlay();
	
	// 상점에서 생성된 아이템이 아닌 경우 파괴 -> 상점에서만 생성되는 아이템으로 정의
	if (!IsItemSetupCorrectly())
	{
		Destroy();
	}
}

void AShopItemActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideInfoWidget();
	Super::EndPlay(EndPlayReason);
}

bool AShopItemActor::IsValidStock(const FDataTableRowHandle& ItemRow, const FDataTableRowHandle& CostItemRow,
                                  int32 CostCount)
{
	if (CostCount <= 0 || ItemRow.IsNull() || CostItemRow.IsNull())
	{
		return false;
	}
	
	const FItemDataTable* ItemData = ItemRow.GetRow<FItemDataTable>(TEXT("AShopItemActor::IsValidStock"));
	const FItemDataTable* CostItemData = CostItemRow.GetRow<FItemDataTable>(TEXT("AShopItemActor::IsValidStock"));
	if (!ItemData || !CostItemData)
	{
		return false;
	}
	
	const bool bSellable = ItemData->ItemType == EItemType::Equipment || ItemData->ItemType == EItemType::Consumable;
	
	// 판매하는 아이템은 장비 혹은 소비 아이템이고 교환 재료는 기타 아이템이어야 함
	return bSellable && CostItemData->ItemType == EItemType::Misc;
}

void AShopItemActor::SetupStock(const FDataTableRowHandle& InCostItemRow, int32 InCostCount)
{
	CostItemRow = InCostItemRow;
	CostItemCount = InCostCount;
}

void AShopItemActor::HandleInventoryInteract(UInventoryComponent* UserInvComponent)
{
	if (nullptr == GetItemComponent())
	{
		return;
	}
	
	UItemData* Item = GetItemComponent()->GetItemData();
	if (nullptr == UserInvComponent || nullptr == Item)
	{
		return;
	}
	
	// 아이템 구매를 위한 지불해야 할 아이템과 구매 후 받을 아이템 설정
	const TArray<FInventoryItemRequirement> Pay = {FInventoryItemRequirement(CostItemRow.RowName, CostItemCount)};
	const TArray<UItemData*> Receive = {Item};
	
	if (UserInvComponent->TryApplyChange(Pay, Receive))
	{
		HideInfoWidget();
		// 아이템을 인벤토리로 넣으므로 월드의 액터는 제거
		Destroy();
	}
}

void AShopItemActor::OnInventoryFocusBegin(UInventoryComponent* PlayerInvComponent, FKey InteractionKey)
{
	IInventoryInteractInterface::OnInventoryFocusBegin(PlayerInvComponent, InteractionKey);
	
	HideInfoWidget();
	
	const UItemComponent* ItemComp = GetItemComponent();
	if (!InfoWidgetClass || !PlayerInvComponent || !ItemComp || !ItemComp->GetItemData())
	{
		return;
	}
	
	const APawn* PlayerPawn = Cast<APawn>(PlayerInvComponent->GetOwner());
	if (nullptr == PlayerPawn)
	{
		return;
	}
	
	APlayerController* PlayerController = Cast<APlayerController>(PlayerPawn->GetController());
	if (nullptr == PlayerController)
	{
		return;
	}
	
	InfoWidget = CreateWidget<UShopItemInfoWidget>(PlayerController, InfoWidgetClass);
	if (!InfoWidget)
	{
		return;
	}
	
	InfoWidget->SetShopItemInfo(ItemComp->GetItemData(), CostItemRow, CostItemCount);
	InfoWidget->AddToViewport(4);
}

void AShopItemActor::OnInventoryFocusEnd(UInventoryComponent* PlayerInvComponent)
{
	IInventoryInteractInterface::OnInventoryFocusEnd(PlayerInvComponent);
	
	HideInfoWidget();
}

bool AShopItemActor::IsItemSetupCorrectly() const
{
	const UItemComponent* ItemComp = GetItemComponent();
	return ItemComp && IsValidStock(ItemComp->ItemRow, CostItemRow, CostItemCount);
}

void AShopItemActor::HideInfoWidget()
{
	if (InfoWidget)
	{
		InfoWidget->RemoveFromViewport();
		InfoWidget = nullptr;
	}
}
