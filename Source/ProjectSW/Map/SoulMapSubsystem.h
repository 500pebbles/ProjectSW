

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/PMGResult.h"
#include "SoulMapSubsystem.generated.h"

class APMGMapBuilder;
class ASoulPackedLevelActor;
class ASoulRoomDoor;

/**
 * 
 */
UCLASS()
class PROJECTSW_API USoulMapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	const ASoulPackedLevelActor* FindRoom(int32 RoomId) const;

	FORCEINLINE bool HasMapData() const { return !Rooms.IsEmpty(); }
	FORCEINLINE const FPMGResult& GetMapResult() const { return MapResult; }
	FORCEINLINE int32 GetCurrentRoomId() const { return CurrentRoomId; }

private:
	UFUNCTION()
	void HandleGenerationFinished(const FPMGResult& Result, const TArray<FString>& SpawnErrors);

	void HandleRoomEntered(ASoulPackedLevelActor* Room);

	void HandleCombatFinished(ASoulPackedLevelActor* Room);

	void InitDoors(const FPMGResult& Result, const TMap<FName, AActor*>& SpawnedByName, const FString& BuilderName);
	void SetRoomDoorsClosed(int32 RoomId, bool bClosed);

private:
	UPROPERTY()
	TWeakObjectPtr<APMGMapBuilder> MapBuilder;

	UPROPERTY()
	FPMGResult MapResult;

	UPROPERTY()
	TMap<int32, TObjectPtr<ASoulPackedLevelActor>> Rooms;

	UPROPERTY()
	TArray<TObjectPtr<ASoulRoomDoor>> Doors;

	int32 CurrentRoomId = INDEX_NONE;
};
