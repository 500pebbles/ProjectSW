

#pragma once

#include "CoreMinimal.h"
#include "PackedLevelActor/PackedLevelActor.h"
#include "SoulPackedLevelActor.generated.h"

class UBoxComponent;
class ASoulPackedLevelActor;
struct FPMGResultRoom;

DECLARE_MULTICAST_DELEGATE_OneParam(FSoulRoomEnteredSignature, ASoulPackedLevelActor*);

/**
 * 
 */
UCLASS()
class PROJECTSW_API ASoulPackedLevelActor : public APackedLevelActor
{
	GENERATED_BODY()

public:
	ASoulPackedLevelActor();


	void InitRoom(const FPMGResultRoom& Room, float CellSize);
	void SetTriggerEnabled(bool bEnabled);
	void MarkVisited();

	FORCEINLINE bool IsVisited() const { return bVisited; }
	FORCEINLINE int32 GetRoomId() const { return RoomId; }

	FSoulRoomEnteredSignature OnPlayerEntered;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);


private:
	UPROPERTY(VisibleAnywhere, Category = "Room")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	float TriggerInset = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	float TriggerHalfHeight = 300.f;

	int32 RoomId = INDEX_NONE;
	bool bVisited = false;
};
