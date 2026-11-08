#include "Run/Interact/SoulItemLibrary.h"
#include "Run/Core/SoulRunConfig.h"
#include "Run/Core/SoulRunGameComponent.h"
#include "Run/Flow/SoulFloorDirectorComponent.h"
#include "Utility/SoulLog.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Item/ItemActorBase.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

AActor* USoulItemLibrary::SpawnPickup(UObject* WorldContextObject, TSubclassOf<AItemActorBase> PickupClass, const FDataTableRowHandle& ItemRow, const FVector& Location)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World || !PickupClass || ItemRow.IsNull())
		return nullptr;

	// 몹 위치는 캡슐 절반만큼 떠 있으므로 바닥에 투영한다
	FVector Ground = Location;
	if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		FNavLocation NavLocation;
		if (NavSys->ProjectPointToNavigation(Location, NavLocation, FVector(400.f, 400.f, 600.f)))
		{
			Ground = NavLocation.Location;
		}
	}

	const FTransform Transform(FRotator::ZeroRotator, Ground);
	AItemActorBase* Pickup = World->SpawnActorDeferred<AItemActorBase>(PickupClass, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Pickup)
		return nullptr;
	Pickup->SetupItem(ItemRow);
	Pickup->FinishSpawning(Transform);

	if (USoulFloorDirectorComponent* Director = USoulFloorDirectorComponent::Get(World))
	{
		Director->RegisterFloorActor(Pickup);
	}
	SW_LOG("[ITEM] spawn %s row=%s", *Pickup->GetName(), *ItemRow.RowName.ToString());
	return Pickup;
}

#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorldAndArgs GSoulItemSpawn(
	TEXT("soul.Item.Spawn"),
	TEXT("soul.Item.Spawn <행 이름> : DT_RunItems 행을 폰 앞 150cm에 스폰한다"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(World);
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
		if (!Config || !Config->ItemTable || !Pawn || Args.IsEmpty())
		{
			SW_LOG_WARNING("soul.Item.Spawn : DA_RunConfig.ItemTable 또는 폰 없음");
			return;
		}
		FDataTableRowHandle Row;
		Row.DataTable = Config->ItemTable;
		Row.RowName = FName(*Args[0]);
		const TSubclassOf<AItemActorBase> PickupClass = Args.Num() > 1 && Args[1].Equals(TEXT("Weapon"), ESearchCase::IgnoreCase) ? Config->WeaponPickupClass : Config->ItemPickupClass;
		USoulItemLibrary::SpawnPickup(World, PickupClass, Row, Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 150.f);
	}));
#endif
