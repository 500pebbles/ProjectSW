#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "SoulLootTable.generated.h"

class AItemActorBase;

/** 드랍 후보 하나 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulLootEntry
{
	GENERATED_BODY()

	/** DT_RunItems 행 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FDataTableRowHandle ItemRow;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	float Weight = 1.f;

	/** 이 층 범위에서만 나온다 (0이면 제한 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	int32 MinFloor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	int32 MaxFloor = 0;

	/** 바닥에 놓을 액터 (BP_ItemPickup, BP_WeaponPickup) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TSubclassOf<AItemActorBase> PickupClass;
};

/** 한 번의 판정. 성공하면 후보 중 가중치로 하나 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulLootRoll
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", ClampMax = "1"))
	float RollChance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TArray<FSoulLootEntry> Entries;
};

/** 드랍 표 (DA_Loot_Chest, _Boss, _Mob). Roll마다 독립 판정 */
UCLASS(BlueprintType)
class PROJECTSW_API USoulLootTable : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TArray<FSoulLootRoll> Rolls;

	/** 판정해 위치 주변에 픽업을 스폰한다. 스폰한 액터는 Director에 층 액터로 등록한다 */
	TArray<AActor*> SpawnLoot(UWorld* World, const FVector& Origin, int32 Floor, float ScatterRadius) const;
};
