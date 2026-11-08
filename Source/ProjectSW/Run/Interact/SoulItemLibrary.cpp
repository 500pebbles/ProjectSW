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

const FName USoulItemLibrary::ScorePickupTag(TEXT("SoulScorePickup"));

AActor* USoulItemLibrary::SpawnPickup(UObject* WorldContextObject, TSubclassOf<AItemActorBase> PickupClass, const FDataTableRowHandle& ItemRow, const FVector& Location, const FVector& NavExtent, float FallbackForward)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World || !PickupClass || ItemRow.IsNull())
		return nullptr;

	// 몹 위치는 캡슐 절반만큼 떠 있으므로 바닥에 투영한다
	FVector Ground = Location;
	bool bProjected = false;
	if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World); NavSys && !NavExtent.IsNearlyZero())
	{
		FNavLocation NavLocation;
		if (NavSys->ProjectPointToNavigation(Location, NavLocation, NavExtent))
		{
			Ground = NavLocation.Location;
			bProjected = true;
		}
	}
	if (!bProjected && FallbackForward > 0.f)
	{
		// 투영이 실패하면 플레이어 앞 (플랜 #11)
		if (const APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0))
		{
			Ground = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * FallbackForward - FVector(0.f, 0.f, Pawn->GetDefaultHalfHeight());
			SW_LOG_WARNING("[ITEM] 바닥 투영 실패, 플레이어 앞에 놓음 row=%s", *ItemRow.RowName.ToString());
		}
	}

	const FTransform Transform(FRotator::ZeroRotator, Ground);
	AItemActorBase* Pickup = World->SpawnActorDeferred<AItemActorBase>(PickupClass, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Pickup)
		return nullptr;
	Pickup->SetupItem(ItemRow);
	// 런이 스폰한 것만 줍기 점수 (인벤토리에서 버린 것은 플러그인이 태그 없이 스폰)
	Pickup->Tags.AddUnique(USoulItemLibrary::ScorePickupTag);
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
		USoulItemLibrary::SpawnPickup(World, PickupClass, Row, Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 150.f, FVector(400.f, 400.f, 600.f), 0.f);
	}));
#endif
