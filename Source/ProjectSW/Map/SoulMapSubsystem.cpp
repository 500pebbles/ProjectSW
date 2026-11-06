


#include "Map/SoulMapSubsystem.h"
#include "SoulPackedLevelActor.h"
#include "SoulRoomDoor.h"
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
	Doors.Reset();
	CurrentRoomId = INDEX_NONE;

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
		Room->OnCombatFinished.AddUObject(this, &ThisClass::HandleCombatFinished);
		Rooms.Add(RoomData.Id, Room);
	}

	InitDoors(Result, SpawnedByName, BuilderName);

	if (ASoulPackedLevelActor* EntranceRoom = Rooms.FindRef(Result.EntranceRoomId))
	{
		EntranceRoom->MarkVisited();
		CurrentRoomId = Result.EntranceRoomId;
	}
		

	for (const TPair<int32, TObjectPtr<ASoulPackedLevelActor>>& Pair : Rooms)
	{
		Pair.Value->SetTriggerEnabled(Result.Rooms[Pair.Key].bConnected);
	}
}

void USoulMapSubsystem::HandleRoomEntered(ASoulPackedLevelActor* Room)
{
	CurrentRoomId = Room->GetRoomId();

	if (Room->IsVisited())
		return;

	Room->MarkVisited();

	if (Room->HasCombat())
	{
		SetRoomDoorsClosed(Room->GetRoomId(), true);
		Room->StartCombat();
	}

	SW_LOG("%d 방 진입", Room->GetRoomId());
}

void USoulMapSubsystem::HandleCombatFinished(ASoulPackedLevelActor* Room)
{
	SetRoomDoorsClosed(Room->GetRoomId(), false);
	SW_LOG("%d 방 전투 종료", Room->GetRoomId());
}

void USoulMapSubsystem::InitDoors(const FPMGResult& Result, const TMap<FName, AActor*>& SpawnedByName, const FString& BuilderName)
{
	for (int32 ConnectorIndex = 0; ConnectorIndex < Result.Connectors.Num(); ++ConnectorIndex)
	{
		const FPMGResultConnector& Connector = Result.Connectors[ConnectorIndex];
		
		if (Connector.Type != EPMGConnectorType::Door)
			continue;

		const FName DoorName(*FString::Printf(TEXT("%s_Connector_%d"), *BuilderName, ConnectorIndex));

		ASoulRoomDoor* Door = Cast<ASoulRoomDoor>(SpawnedByName.FindRef(DoorName));

		if (!Door)
		{
			if (Connector.VariantIndex != INDEX_NONE)
			{
				SW_LOG_WARNING("USoulMapSubsystem::InitDoors : 문 액터를 찾지 못함: %s", *DoorName.ToString());
			}
			continue;
		}

		const FPMGResultDoor* DoorData = Result.Doors.FindByPredicate([&Connector](const FPMGResultDoor& Candidate)
			{
				return (Candidate.CellA == Connector.CellA) && (Candidate.CellB == Connector.CellB);
			});

		if (!DoorData)
		{
			SW_LOG_WARNING("USoulMapSubsystem::InitDoors : 문 데이터를 찾지 못함: %s", *DoorName.ToString());
			continue;
		}

		Door->InitDoor(DoorData->RoomA, DoorData->RoomB);
		Doors.Add(Door);
	}

	SW_LOG("문 %d개 초기화(PMGResult의 문 %d개)", Doors.Num(), Result.Doors.Num());
}

void USoulMapSubsystem::SetRoomDoorsClosed(int32 RoomId, bool bClosed)
{
	for (ASoulRoomDoor* Door : Doors)
	{
		if (IsValid(Door) && Door->ConnectsRoom(RoomId))
		{
			Door->SetClosed(bClosed);
		}
	}
}

const ASoulPackedLevelActor* USoulMapSubsystem::FindRoom(int32 RoomId) const
{
	return Rooms.FindRef(RoomId);
}
