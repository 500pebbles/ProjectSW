

#pragma once

#include "CoreMinimal.h"
#include "PackedLevelActor/PackedLevelActor.h"
#include "SoulPackedLevelActor.generated.h"

class UBoxComponent;
class ASoulPackedLevelActor;
class ASoulCharacterEnemy;
class USoulEnemySet;
class USoulRoomSpawnPointComponent;
struct FPMGResultRoom;

DECLARE_MULTICAST_DELEGATE_OneParam(FSoulRoomEnteredSignature, ASoulPackedLevelActor*);
DECLARE_MULTICAST_DELEGATE_OneParam(FSoulRoomCombatFinishedSignature, ASoulPackedLevelActor*);

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

	bool HasCombat() const;
	void StartCombat();

	FSoulRoomEnteredSignature OnPlayerEntered;
	FSoulRoomCombatFinishedSignature OnCombatFinished;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	const USoulEnemySet* GetEnemySetFor(const USoulRoomSpawnPointComponent* SpawnPoint) const;
	void FinishCombat();

	UFUNCTION()
	void HandleEnemyGone(AActor* Enemy);


private:
	UPROPERTY(VisibleAnywhere, Category = "Room")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	float TriggerInset = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	float TriggerHalfHeight = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<USoulEnemySet> DefaultEnemySet;

	UPROPERTY()
	TArray<TObjectPtr<ASoulCharacterEnemy>> AliveEnemies;

	bool bInCombat = false;

	int32 RoomId = INDEX_NONE;
	bool bVisited = false;
};
