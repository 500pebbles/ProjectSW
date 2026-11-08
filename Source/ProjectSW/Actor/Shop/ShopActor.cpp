// 


#include "ShopActor.h"

#include "ShopItemActor.h"
#include "Engine/World.h"
#include "Item/Data/ItemData.h"


// Sets default values
AShopActor::AShopActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	
	for (int32 i = 0; i < ShopItemCount; ++i)
	{
		const FName SlotName = *FString::Printf(TEXT("SlotPoint%d"), i);
		USceneComponent* SlotPoint = CreateDefaultSubobject<USceneComponent>(SlotName);
		SlotPoint->SetupAttachment(SceneRoot);
		SlotPoint->SetRelativeLocation(FVector(0.0f, 80.0f * (i-1), 0.0f));
		SlotPoints.Add(SlotPoint);
	}
}

// Called when the game starts or when spawned
void AShopActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (!ShopItemClass)
	{
		return;
	}

	int32 SpawnedCount = 0;
	for (int32 i = 0; i < ShopItemCount; ++i)
	{
		UE_LOG(LogTemp, Log, TEXT("Trying Spawn Item"));
		
		FShopStockEntry Entry;
		bool bHasEntry = false;
		
		// 이미 지정한 아이템이 있는 경우 우선 사용
		if (Stock.IsValidIndex(i))
		{
			if (AShopItemActor::IsValidStock(Stock[i].ItemRow, Stock[i].CostItemRow, Stock[i].CostItemCount))
			{
				Entry = Stock[i];
				bHasEntry = true;
			}
		}
		
		if (!bHasEntry)
		{
			bHasEntry = BuildRandomEntry(Entry);
		}
		
		if (!bHasEntry)
		{
			UE_LOG(LogTemp, Warning, TEXT("상점 아이템 생성 실패"));
			continue;
		}
		
		if (SpawnShopItem(i, Entry))
		{
			++SpawnedCount;
		}
	}
}

bool AShopActor::SpawnShopItem(int32 SlotIndex, const FShopStockEntry& Entry)
{
	// 교환에 필요한 아이템 정보가 맞지 않으면 아이템 생성 안함
	if (!AShopItemActor::IsValidStock(Entry.ItemRow, Entry.CostItemRow, Entry.CostItemCount))
	{
		return false;
	}
	
	if (!SlotPoints.IsValidIndex(SlotIndex) || !SlotPoints[SlotIndex])
	{
		return false;
	}
	
	const FTransform SpawnTransform = SlotPoints[SlotIndex]->GetComponentTransform();
	
	// 지연생성으로 BeginPlay전에 아이템 행과 교환 조건을 먼저 설정
	AShopItemActor* ShopItem = GetWorld()->SpawnActorDeferred<AShopItemActor>(ShopItemClass, SpawnTransform, 
		this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	
	if (!ShopItem)
	{
		return false;
	}
	
	ShopItem->SetupItem(Entry.ItemRow);
	ShopItem->SetupStock(Entry.CostItemRow, Entry.CostItemCount);
	ShopItem->FinishSpawning(SpawnTransform);
	
	// 아이템이 가판대와 붙어있도록 함
	ShopItem->AttachToComponent(SlotPoints[SlotIndex], FAttachmentTransformRules::KeepWorldTransform);
	return true;
}

bool AShopActor::BuildRandomEntry(FShopStockEntry& OutEntry) const
{
	if (!ItemTable)
	{
		return false;
	}
	
	TArray<FName> ItemRows;
	TArray<FName> CostRows;
	CollectSpecificRows(false, ItemRows);
	CollectSpecificRows(true, CostRows);
	
	if (ItemRows.IsEmpty() || CostRows.IsEmpty())
	{
		return false;
	}
	
	OutEntry.ItemRow.DataTable = ItemTable;
	OutEntry.ItemRow.RowName = ItemRows[FMath::RandRange(0, ItemRows.Num() - 1)];
	OutEntry.CostItemRow.DataTable = ItemTable;
	OutEntry.CostItemRow.RowName = CostRows[FMath::RandRange(0, CostRows.Num() - 1)];
	OutEntry.CostItemCount = FMath::RandRange(RandomCostCountMin, FMath::Max(RandomCostCountMin, RandomCostCountMax));
	
	return true;
}

void AShopActor::CollectSpecificRows(bool bForCost, TArray<FName>& OutRows) const
{
	if (!ItemTable)
	{
		return;
	}
	
	for (const FName& RowName : ItemTable->GetRowNames())
	{
		const FItemDataTable* Row = ItemTable->FindRow<FItemDataTable>(RowName, TEXT("AShopActor::CollectSpecificRows"));
		if (!Row)
		{
			continue;
		}
		
		const bool bMatch = bForCost ? Row->ItemType == EItemType::Misc : Row->ItemType == EItemType::Equipment || Row->ItemType == EItemType::Consumable;
		
		if (bMatch)
		{
			OutRows.Add(RowName);
		}
	}
}

