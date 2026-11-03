


#include "SoulPackedLevelActor.h"
#include "Components/BoxComponent.h"
#include "SoulRoomSpawnPointComponent.h"
#include "Character/SoulEnemySet.h"
#include "Character/SoulCharacterPlayer.h"
#include "Data/PMGResult.h"
#include "Utility/SoulLog.h"

ASoulPackedLevelActor::ASoulPackedLevelActor()
{
	Trigger = CreateDefaultSubobject<UBoxComponent>("Trigger");
	Trigger->SetupAttachment(RootComponent);
	Trigger->SetCollisionProfileName("Trigger");
	Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASoulPackedLevelActor::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleTriggerBeginOverlap);
}

void ASoulPackedLevelActor::InitRoom(const FPMGResultRoom& Room, float CellSize)
{
	RoomId = Room.Id;

	FVector2D Cells(1.0, 1.0);
	switch (PMG::GetRoomSize(Room.Shape))
	{
	case EPMGRoomSize::Dual:
		Cells = FVector2D(2.0, 1.0);
		break;
	case EPMGRoomSize::Quad:
		Cells = FVector2D(2.0, 2.0);
		break;
	}

	const double HalfX = Cells.X * CellSize * 0.5 - TriggerInset;
	const double HalfY = Cells.Y * CellSize * 0.5 - TriggerInset;
	Trigger->SetBoxExtent(FVector(HalfX, HalfY, TriggerHalfHeight));
	Trigger->SetRelativeLocation(FVector(0.0, 0.0, TriggerHalfHeight));
}

void ASoulPackedLevelActor::SetTriggerEnabled(bool bEnabled)
{
	Trigger->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void ASoulPackedLevelActor::MarkVisited()
{
	bVisited = true;
	SetTriggerEnabled(false);
}

bool ASoulPackedLevelActor::HasCombat() const
{
	TArray<USoulRoomSpawnPointComponent*> SpawnPoints;
	GetComponents(SpawnPoints);

	for (const USoulRoomSpawnPointComponent* SpawnPoint : SpawnPoints)
	{
		if (GetEnemySetFor(SpawnPoint))
		{
			return true;
		}
	}

	return false;
}

void ASoulPackedLevelActor::StartCombat()
{
}

void ASoulPackedLevelActor::HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<ASoulCharacterPlayer>(OtherActor))
	{
		OnPlayerEntered.Broadcast(this);
	}
}

const USoulEnemySet* ASoulPackedLevelActor::GetEnemySetFor(const USoulRoomSpawnPointComponent* SpawnPoint) const
{
	const USoulEnemySet* EnemySet = SpawnPoint->GetEnemySet() ? SpawnPoint->GetEnemySet() : DefaultEnemySet.Get();
	return EnemySet && !EnemySet->GetEnemyClasses().IsEmpty() ? EnemySet : nullptr;
}

void ASoulPackedLevelActor::FinishCombat()
{
}

void ASoulPackedLevelActor::HandleEnemyGone(AActor* Enemy)
{
}
