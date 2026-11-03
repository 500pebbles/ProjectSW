


#include "Map/SoulMapSubsystem.h"
#include "SoulPackedLevelActor.h"
#include "Build/PMGMapBuilder.h"
#include "Data/PMGSettings.h"
#include "Utility/SoulLog.h"
#include "EngineUtils.h"

void USoulMapSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	TActorIterator<APMGMapBuilder> Iter(&InWorld);
	MapBuilder = Iter ? *Iter : nullptr;
	if (MapBuilder.IsValid())
	{
		MapBuilder->OnGenerationFinished.AddDynamic(this, &ThisClass::HandleGenerationFinished);
	}
}

void USoulMapSubsystem::HandleGenerationFinished(const FPMGResult& Result, const TArray<FString>& SpawnErrors)
{
	Rooms.Reset();

	if (!Result.bSuccess)
		return;

	MapResult = Result;

	TMap<FName, AActor*> SpawnedByName;
	for (AActor* Actor : MapBuilder->SpawnedActors)
	{
		if (IsValid(Actor))
		{
			SpawnedByName.Add(Actor->GetFName(), Actor);
		}
	}

	const FString BuilderName = MapBuilder->GetName();
	const float CellSize = MapBuilder->Settings->CellSize;

	for (const FPMGResultRoom& RoomData : Result.Rooms)
	{
		const FName RoomName(*FString::Printf(TEXT("%s_Room_%d"), *BuilderName, RoomData.Id));
		ASoulPackedLevelActor* Room = Cast<ASoulPackedLevelActor>(SpawnedByName.FindRef(RoomName));

		if (!Room)
		{
			if (RoomData.bConnected && RoomData.VariantIndex != INDEX_NONE)
			{
				SW_LOG_WARNING("USoulMapSubsystem::HandleGenerationFinished : [Room] 방 %d의 액터를 찾지 못함: %s", RoomData.Id, *RoomName.ToString());
			}
			continue;
		}

		Room->InitRoom(RoomData, CellSize);
		Room->OnPlayerEntered.AddUObject(this, &ThisClass::HandleRoomEntered);
		Rooms.Add(RoomData.Id, Room);
	}

	if (ASoulPackedLevelActor* EntranceRoom = Rooms.FindRef(Result.EntranceRoomId))
		EntranceRoom->MarkVisited();

	for (const TPair<int32, TObjectPtr<ASoulPackedLevelActor>>& Pair : Rooms)
	{
		Pair.Value->SetTriggerEnabled(Result.Rooms[Pair.Key].bConnected && !Pair.Value->IsVisited());
	}
}

void USoulMapSubsystem::HandleRoomEntered(ASoulPackedLevelActor* Room)
{
	if (Room->IsVisited())
		return;

	Room->MarkVisited();

	if (Room->HasCombat())
	{
		SW_LOG("HasCombat : true");
	}
	else
	{
		SW_LOG("HasCombat : false");
	}

	SW_LOG("%d 방 진입", Room->GetRoomId());
}
