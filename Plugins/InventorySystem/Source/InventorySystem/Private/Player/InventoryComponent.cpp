// 


#include "Player/InventoryComponent.h"

#include "Blueprint/UserWidget.h"
#include "Item/ItemActorBase.h"
#include "Player/InventoryInputComponent.h"
#include "UI/Inventory/GridInventoryWidget.h"
#include "UI/Inventory/InventoryBase.h"
#include "Item/ItemComponent.h"
#include "Item/Data/ItemData.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InventoryUIClass = UGridInventoryWidget::StaticClass();
}


// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	APawn* PlayerPawn = Cast<APawn>(GetOwner());
	if (IsValid(PlayerPawn))
	{
		LocalGameController = Cast<APlayerController>(PlayerPawn->GetController());
		
		CreateInventoryUIWidget();
	}
	
	// 인터페이스를 통해 의존성 분리
	// InputComponent에서 인터페이스를 상속해 델리게이트의 정의를 반환하고, 여기서 반환받은 정의에 이벤트 구독
	TArray<UActorComponent*> InventoryInputComponent = GetOwner()->GetComponentsByInterface(UInventoryInputInterface::StaticClass());
	for (UActorComponent* InventoryInput : InventoryInputComponent)
	{
		if (IInventoryInputInterface* InputInterface = Cast<IInventoryInputInterface>(InventoryInput))
		{
			InputInterface->GetOnToggleInventoryUI().AddDynamic(this, &UInventoryComponent::ToggleInventoryUI);
			InputInterface->GetOnCollectItem().AddDynamic(this, &UInventoryComponent::CollectItem);
		}
	}
	
	// 인벤토리 데이터 초기화
	//InventoryData.InitializeArray();
}

void UInventoryComponent::CreateInventoryUIWidget()
{
	InventoryUI = CreateWidget<UInventoryBase>(LocalGameController, InventoryUIClass);
	
	if (IsValid(InventoryUI))
	{
		InventoryUI->AddToViewport(5);
	}
}

void UInventoryComponent::ToggleInventoryUI()
{
	InventoryUI->ToggleInventoryUI();

	const FInputModeUIOnly InventoryInputMode;
	const FInputModeGameOnly GameInputMode;
	
	// 인벤토리 UI 닫기
	if (bIsInventoryUIOpen)
	{
		LocalGameController->SetInputMode(GameInputMode);
		LocalGameController->SetShowMouseCursor(false);
		bIsInventoryUIOpen = false;
	}
	// 인벤토리 UI 오픈
	else
	{
		LocalGameController->SetInputMode(InventoryInputMode);
		LocalGameController->SetShowMouseCursor(true);
		bIsInventoryUIOpen = true;
	}
}



void UInventoryComponent::CollectItem(AActor* TargetItem)
{
	if (!IsValid(TargetItem))
	{
		return;
	}
	
	UItemComponent* TargetItemComponent = TargetItem->GetComponentByClass<UItemComponent>();
	
	// 아이템이 추가될 떄 UI에 반영하도록 조건 추가
	const int32 PrevItemCount = InventoryData.ItemDataArray.Num();
	InventoryData.AddItemData(TargetItemComponent);
	
	if(InventoryData.ItemDataArray.Num() > PrevItemCount)
	{
		// 가장 마지막에 추가된 아이템을 전달
		OnInventoryItemAdded.Broadcast(&InventoryData.ItemDataArray.Last());
	}
	
	// 월드 상에서 아이템 제거
	TargetItem->Destroy();
}

void UInventoryComponent::UseItem(const FGuid& ItemID)
{
	const FInventoryDataEntry* UsedItemEntry = InventoryData.FindItemWithID(ItemID);
	if (!UsedItemEntry)
	{
		return;
	}
	
	UItemData* UsedItem = UsedItemEntry->GetItem();
	if (!UsedItem)
	{
		return;
	}
	
	if (!UsedItem->UseItem(GetOwner()))
	{
		return;
	}
	
	if (UsedItem->IsConsumedOnUse())
	{
		// 소모성 아이템이면 아이템 제거
		RemoveItem(ItemID);
	}
}

int32 UInventoryComponent::RemoveItem(const FGuid& InItemID, int32 RemoveCount)
{
	const FRemoveResult Result = InventoryData.RemoveItemData(InItemID, RemoveCount);
	if (Result.RemovedCount <= 0)
	{
		return 0;
	}

	if (Result.RemainingItemCount <= 0)
	{
		OnInventoryItemRemoved.Broadcast(InItemID);
	}
	else
	{
		OnItemQuantityChanged.Broadcast(InItemID, Result.RemainingItemCount);
	}
	
	return Result.RemovedCount;
}

int32 UInventoryComponent::DropItem(const FGuid& InID, int32 Count)
{
	if (Count <= 0) { return 0; }

	const FInventoryDataEntry* Entry = InventoryData.FindItemWithID(InID);
	if (!Entry || !Entry->GetItem()) { return 0; }

	// 제거 전에 필요한 값을 복사해 둔다. 이후 Entry 포인터는 쓰지 않는다.
	const FDataTableRowHandle ItemRow = Entry->GetItem()->GetRowHandle(); // 가정
	const int32 DropQuantity = FMath::Min(Count, Entry->GetQuantity());

	if (!SpawnDroppedItem(ItemRow))
	{
		return 0; // 스폰 실패 시 인벤토리는 그대로
	}
	return RemoveItem(InID, DropQuantity);
}

bool UInventoryComponent::SpawnDroppedItem(const FDataTableRowHandle& InItemRow)
{
	UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();
	if (!World || !OwnerActor || InItemRow.IsNull())
	{
		return false;
	}

	const TSubclassOf<AItemActorBase> SpawnClass = ItemActorClass ? ItemActorClass : TSubclassOf<AItemActorBase>(AItemActorBase::StaticClass());
	const FTransform SpawnTransform = CalculateDropPosition(*OwnerActor);

	AItemActorBase* Spawned = World->SpawnActorDeferred<AItemActorBase>(
		SpawnClass, SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Spawned)
	{
		return false;
	}

	Spawned->SetupItem(InItemRow);
	Spawned->FinishSpawning(SpawnTransform);
	return true;
}

FTransform UInventoryComponent::CalculateDropPosition(const AActor& OwnerActor) const
{
	const FVector Desired = OwnerActor.GetActorLocation() + OwnerActor.GetActorForwardVector() * DropDistance;

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(&OwnerActor);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, Desired + FVector(0, 0, 100.f), Desired - FVector(0, 0, 500.f), ECC_Visibility, QueryParams);

	return FTransform(FRotator::ZeroRotator, bHit ? FVector(Hit.ImpactPoint) : Desired);
}



bool UInventoryComponent::MoveItemPosition(const FGuid& TargetID, FIntPoint TargetPosition)
{
	return InventoryData.MoveItemPosition(TargetID, TargetPosition);
}

bool UInventoryComponent::CheckSlotAvailable(const FGuid& SlotID, FIntPoint TargetPosition) const
{
	const FInventoryDataEntry* CurrentItem = FindItemWithID(SlotID);
	if (!CurrentItem)
	{
		return false;
	}
	return InventoryData.CheckSlotAvailable(TargetPosition, CurrentItem->GetOccupiedSize(), SlotID);
}

const FInventoryDataEntry* UInventoryComponent::FindItemWithID(const FGuid& ItemID) const
{
	return InventoryData.FindItemWithID(ItemID);
}
