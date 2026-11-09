#include "Run/Equipment/SoulLootTable.h"
#include "Run/Flow/SoulFloorDirectorComponent.h"
#include "Run/Interact/SoulItemLibrary.h"
#include "Utility/SoulLog.h"

#include "Engine/World.h"
#include "Item/ItemActorBase.h"
#include "NavigationSystem.h"

TArray<AActor*> USoulLootTable::SpawnLoot(UWorld* World, const FVector& Origin, int32 Floor, const FSoulDropPlacement& Placement) const
{
	TArray<AActor*> Spawned;
	if (!World)
		return Spawned;

	int32 Index = 0;
	for (const FSoulLootRoll& Roll : Rolls)
	{
		if (FMath::FRand() >= Roll.RollChance)
			continue;

		float Total = 0.f;
		TArray<const FSoulLootEntry*> Valid;
		for (const FSoulLootEntry& Entry : Roll.Entries)
		{
			if (Entry.Weight <= 0.f || !Entry.PickupClass || Entry.ItemRow.IsNull())
				continue;
			if ((Entry.MinFloor > 0 && Floor < Entry.MinFloor) || (Entry.MaxFloor > 0 && Floor > Entry.MaxFloor))
				continue;
			Valid.Add(&Entry);
			Total += Entry.Weight;
		}
		if (Valid.IsEmpty())
			continue;

		float Pick = FMath::FRandRange(0.f, Total);
		const FSoulLootEntry* Chosen = Valid.Last();
		for (const FSoulLootEntry* Entry : Valid)
		{
			Pick -= Entry->Weight;
			if (Pick <= 0.f)
			{
				Chosen = Entry;
				break;
			}
		}

		// 여러 개면 서로 띄운다 (플랜 #11 드랍 위치)
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const FVector Offset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * (Placement.ScatterRadius > 0.f ? FMath::FRandRange(0.f, Placement.ScatterRadius) : 0.f) + FVector(Placement.Spacing * Index, 0.f, 0.f);
		if (AActor* Pickup = USoulItemLibrary::SpawnPickup(World, Chosen->PickupClass, Chosen->ItemRow, Origin + Offset, Placement.NavExtent, Placement.FallbackForward))
		{
			Spawned.Add(Pickup);
			++Index;
		}
	}
	return Spawned;
}
