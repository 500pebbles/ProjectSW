

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulRoomDoor.generated.h"

UCLASS()
class PROJECTSW_API ASoulRoomDoor : public AActor
{
	GENERATED_BODY()
	
public:	
	void InitDoor(int32 InRoomA, int32 InRoomB);

	FORCEINLINE bool ConnectsRoom(int32 RoomId) const { return RoomA == RoomId || RoomB == RoomId; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Room")
	void SetClosed(bool bClosed);

private:
	int32 RoomA = INDEX_NONE;
	int32 RoomB = INDEX_NONE;
};
