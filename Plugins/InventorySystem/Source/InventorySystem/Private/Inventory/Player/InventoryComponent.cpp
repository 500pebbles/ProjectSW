// 


#include "Inventory/Player/InventoryComponent.h"

#include "Blueprint/UserWidget.h"
#include "Inventory/Interface/InventoryInputInterface.h"
#include "Inventory/UI/InventoryHUDWidget.h"
#include "Item/ItemActorBase.h"
#include "Inventory/UI/Inventory/GridInventoryWidget.h"
#include "Inventory/UI/Inventory/InventoryBase.h"
#include "Item/ItemComponent.h"
#include "Item/Data/ItemData.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;

	InventoryUIClass = UGridInventoryWidget::StaticClass();
	
	ItemActorClass = AItemActorBase::StaticClass();
}

void UInventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();
	
	if (AActor* Owner = GetOwner())
	{
		EquipmentComponent = Owner->GetComponentByClass<UEquipmentComponent>();
		if (!IsValid(EquipmentComponent))
		{
			EquipmentComponent = NewObject<UEquipmentComponent>(Owner);
			EquipmentComponent->RegisterComponent();
		}
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
			InputInterface->GetOnUseQuickSlotItem().AddDynamic(this, &UInventoryComponent::UseQuickSlotItem);
			InputInterface->GetOnTraceItemChanged().AddDynamic(this, &UInventoryComponent::HandleTraceItemChanged);
			InputInterface->GetSwitchQuickSlotItem().AddDynamic(this, &UInventoryComponent::CycleQuickSlot);
			InputInterface->GetToggleEquipmentUI().AddDynamic(this, &UInventoryComponent::ToggleEquipmentUI);
			InputInterface->GetSetupQuickSlotButtonText().AddDynamic(this, &UInventoryComponent::SetupQuickSlotButtonText);
		}
	}
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
		CreateInventoryHUDWidget();
		CreateEquipmentUIWidget();
	}
	
	// 인벤토리 데이터 초기화
	//InventoryData.InitializeArray();
	
	if (CachedUseKey.IsValid() && CachedPrevKey.IsValid() && CachedNextKey.IsValid())
	{
		InventoryHUDWidget->SetQuickSlotButtonText(CachedUseKey, CachedPrevKey, CachedNextKey);
	}
	
	RefreshQuickSlot();
	HandleTraceItemChanged(false, TEXT("None"), nullptr);
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
	if(InventoryData.AddItemData(TargetItemComponent))
	{
		// 월드 상에서 아이템 제거
		TargetItem->Destroy();
		
		if(InventoryData.ItemDataArray.Num() > PrevItemCount)
		{
			// 가장 마지막에 추가된 아이템을 전달
			OnInventoryItemAdded.Broadcast(&InventoryData.ItemDataArray.Last());
		}
	
		RefreshQuickSlot();
	}
}

// Testcode
void UInventoryComponent::UseQuickSlotItem()
{
	// 현재 선택된 퀵슬롯 아이템의 ID를 이용해 아이템 사용
	if (const FInventoryDataEntry* Entry = FindFirstConsumableKind(SeletedQuickSlotItemKind))
	{
		UseItem(Entry->GetID());
	}
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
	
	RefreshQuickSlot();
	
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


void UInventoryComponent::CreateInventoryHUDWidget()
{
	if (!InventoryHUDWidgetClass)
	{
		return;
	}
	
	InventoryHUDWidget = CreateWidget<UInventoryHUDWidget>(LocalGameController, InventoryHUDWidgetClass);
	
	if (IsValid(InventoryHUDWidget))
	{
		InventoryHUDWidget->AddToViewport();
	}
}

void UInventoryComponent::HandleTraceItemChanged(bool bHasTarget, FKey InputKey, AActor* TargetItemActor)
{
	if (!IsValid(InventoryHUDWidget))
	{
		return;
	}
	
	if (!bHasTarget)
	{
		InventoryHUDWidget->HideInteractionMessage();
		return;
	}
	
	// 아이템이 있는 경우 아이템의 이름 전달
	FText ItemName = FText::GetEmpty();
	if (IsValid(TargetItemActor))
	{
		UItemComponent* ItemComponent = Cast<UItemComponent>(TargetItemActor->GetComponentByClass(UItemComponent::StaticClass()));
		if (ItemComponent)
		{
			ItemName = ItemComponent->GetItemData()->GetItemName();	
		}
	}

	InventoryHUDWidget->ShowInteractionMessage(InputKey, ItemName);
}

void UInventoryComponent::SetupQuickSlotButtonText(FKey UseKey, FKey SwitchPrevKey, FKey SwitchNextKey)
{
	CachedUseKey = UseKey;
	CachedPrevKey = SwitchPrevKey;
	CachedNextKey = SwitchNextKey;
	
	if (UseKey.IsValid() && SwitchPrevKey.IsValid() && SwitchNextKey.IsValid() && IsValid(InventoryHUDWidget))
	{
		InventoryHUDWidget->SetQuickSlotButtonText(UseKey, SwitchPrevKey, SwitchNextKey);
	}
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

	bIsInventoryUIOpen = !bIsInventoryUIOpen;
	UpdateUIInputMode();
}

void UInventoryComponent::CreateEquipmentUIWidget()
{
	if (!EquipUIClass)
	{
		return;
	}
	
	EquipUI = CreateWidget<UUserWidget>(LocalGameController, EquipUIClass);
	if (IsValid(EquipUI))
	{
		EquipUI->AddToViewport(6);
		EquipUI->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UInventoryComponent::ToggleEquipmentUI()
{
	if (!IsValid(EquipUI))
	{
		return;
	}
	
	bIsEquipUIOpen = !bIsEquipUIOpen;
	EquipUI->SetVisibility(bIsEquipUIOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	
	UpdateUIInputMode();
}

void UInventoryComponent::UpdateUIInputMode()
{
	if (!IsValid(LocalGameController))
	{
		return;
	}
	
	// 둘 중 하나라도 열려 있으면 마우스를 쓸 수 있는 모드 유지
	if (bIsInventoryUIOpen || bIsEquipUIOpen)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		LocalGameController->SetInputMode(InputMode);
		LocalGameController->SetShowMouseCursor(true);
	}
	else
	{
		LocalGameController->SetInputMode(FInputModeGameOnly());
		LocalGameController->SetShowMouseCursor(false);
	}
}


TArray<FName> UInventoryComponent::GetConsumableItemKinds() const
{
	TArray<FName> Kinds;
	for (const FInventoryDataEntry& Entry : InventoryData.ItemDataArray)
	{
		const UItemData* Item = Entry.GetItem();
		// 아이템이 소비 타입인 경우 퀵슬롯에 자동 등록
		if (Item && Item->GetItemType() == EItemType::Consumable)
		{
			// AddUnique라서 중복 등록 안함
			Kinds.AddUnique(Item->GetRowName());
		}
	}
	return Kinds;
}

const FInventoryDataEntry* UInventoryComponent::FindFirstConsumableKind(FName Kind) const
{
	if (Kind.IsNone())
	{
		return nullptr;
	}
	
	return InventoryData.ItemDataArray.FindByPredicate([Kind](const FInventoryDataEntry& Entry)
	{
		const UItemData* Item = Entry.GetItem();
		// 파라미터로 전달받은 타입이 인벤토리 데이터 내에 존재하면 해당 타입의 RowName(데이터 테이블고유 값)을 갖고옴
		return Item && Item->GetRowName() == Kind;
	});
}

void UInventoryComponent::RefreshQuickSlot()
{
	const TArray<FName> Kinds = GetConsumableItemKinds();
	// 소비 아이템이 하나도 없으면 아이템이 없으므로 None 전달
	if (Kinds.IsEmpty())
	{
		SeletedQuickSlotItemKind = NAME_None;
	}
	
	// 소비 아이템 있으면 기본값으로 0번 인덱스에 해당하는 이름 값 가져옴
	else if (!Kinds.Contains(SeletedQuickSlotItemKind))
	{
		SeletedQuickSlotItemKind = Kinds[0];
	}

	UTexture2D* Icon = nullptr;
	if (const FInventoryDataEntry* Entry = FindFirstConsumableKind(SeletedQuickSlotItemKind))
	{
		// 현재 퀵슬롯에 선택된 아이템에 해당하는 소비 아이템이 인벤토리에 존재하면 해당 아이콘으로 퀵슬롯 아이콘 변경
		if (UItemData* Item = Entry->GetItem())
		{
			Icon = Item->GetItemIcon();
		}
	}

	if (IsValid(InventoryHUDWidget))
	{
		InventoryHUDWidget->SetQuickSlotIcon(Icon);
	}
}

void UInventoryComponent::CycleQuickSlot(bool bIsHeadingNext)
{
	const TArray<FName> Kinds = GetConsumableItemKinds();
	if (Kinds.Num() <= 1)
	{
		return;
	}
	
	const int32 Index = Kinds.IndexOfByKey(SeletedQuickSlotItemKind); // 없으면 -1
	
	if (bIsHeadingNext)
	{
		SeletedQuickSlotItemKind = Kinds[(Index + 1) % Kinds.Num()];
	}
	else
	{
		SeletedQuickSlotItemKind = Kinds[(Index - 1 + Kinds.Num()) % Kinds.Num()];
	}
	
	RefreshQuickSlot();
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

bool UInventoryComponent::CheckSlotAvailableEquipment(const UItemData* Item, FIntPoint TargetPosition) const
{
	if (!IsValid(Item))
	{
		return false;
	}
	
	return InventoryData.CheckSlotAvailable(TargetPosition, Item->GetItemSize(), FGuid());
}

const FInventoryDataEntry* UInventoryComponent::FindItemWithID(const FGuid& ItemID) const
{
	return InventoryData.FindItemWithID(ItemID);
}

bool UInventoryComponent::EquipItem(const FGuid& ItemID)
{
	if (!IsValid(EquipmentComponent))
	{
		return false;
	}
	
	const FInventoryDataEntry* ItemEntry = FindItemWithID(ItemID);
	if (nullptr == ItemEntry)
	{
		return false;
	}
	
	UItemData* ItemData = ItemEntry->GetItem();
	if (nullptr == ItemData)
	{
		return false;
	}
	
	const FIntPoint ItemPosition = ItemEntry->GetGridPosition();
	
	UItemData* PrevItem = EquipmentComponent->GetEquippedItem(ItemData->GetEquipmentType());
	FIntPoint PrevPosition = FIntPoint::NoneValue;
	
	if (PrevItem)
	{
		if (InventoryData.CheckSlotAvailable(ItemPosition, PrevItem->GetItemSize(), ItemID))
		{
			PrevPosition = ItemPosition;
		}
		else
		{
			PrevPosition = InventoryData.FindAvailablePosition(PrevItem, ItemID);
		}
		
		if (PrevPosition.X < 0 || PrevPosition.Y < 0)
		{
			return false;
		}
	}
	
	// 원래 끼고있던 장비를 저장할 지역 변수
	UItemData* ReplacedItem = nullptr;
	if (!EquipmentComponent->Equip(ItemData, ReplacedItem))
	{
		return false;
	}
	
	RemoveItem(ItemID);
	
	if (ReplacedItem)
	{
		AddUnequipedItem(ReplacedItem, PrevPosition);
	}
	
	return true;
}

bool UInventoryComponent::UnequipItem(const FGameplayTag& EquipmentTag, FIntPoint Position)
{
	if (nullptr == EquipmentComponent)
	{
		return false;
	}
	
	UItemData* CurrentEquipment = EquipmentComponent->GetEquippedItem(EquipmentTag);
	if (nullptr == CurrentEquipment)
	{
		return false;
	}
	
	if (!InventoryData.CheckSlotAvailable(Position, CurrentEquipment->GetItemSize(), FGuid()))
	{
		return false;
	}
	
	EquipmentComponent->Unequip(EquipmentTag);
	return AddUnequipedItem(CurrentEquipment, Position);
}

bool UInventoryComponent::DropEquippedItem(const FGameplayTag& EquipmentTag)
{
	if (!IsValid(EquipmentComponent))
	{
		return false;
	}
	
	// 장착된 아이템 없으면 실패
	UItemData* CurrentEquippedItem = EquipmentComponent->GetEquippedItem(EquipmentTag);
	if (!CurrentEquippedItem)
	{
		return false;
	}
	
	if (!SpawnDroppedItem(CurrentEquippedItem->GetRowHandle()))
	{
		return false;
	}
	
	EquipmentComponent->Unequip(EquipmentTag);
	return true;
}

bool UInventoryComponent::AddUnequipedItem(UItemData* Item, FIntPoint Position)
{
	const FInventoryDataEntry* ItemEntry = InventoryData.AddItemDataToPosition(Item, Position);
	if(nullptr == ItemEntry)
	{
		return false;
	}
	
	OnInventoryItemAdded.Broadcast(ItemEntry);
	
	RefreshQuickSlot();
	
	return true;
}
