#include "Run/Equipment/SoulLootDropComponent.h"
#include "Run/Equipment/SoulLootTable.h"
#include "Run/Flow/SoulFloorDirectorComponent.h"
#include "Character/SoulCharacterBase.h"

USoulLootDropComponent::USoulLootDropComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USoulLootDropComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bDropOnDeath && GetWorld()->IsGameWorld())
	{
		if (ASoulCharacterBase* Character = Cast<ASoulCharacterBase>(GetOwner()))
		{
			Character->OnDied.AddUObject(this, &ThisClass::HandleDied);
		}
	}
}

void USoulLootDropComponent::HandleDied(AActor* DeadActor)
{
	DropLoot();
}

void USoulLootDropComponent::DropLoot()
{
	DropLootAt(GetOwner()->GetActorLocation());
}

void USoulLootDropComponent::DropLootAt(const FVector& Location)
{
	// 던전 밖(팀 맵)에서는 드랍하지 않는다
	const USoulFloorDirectorComponent* Director = USoulFloorDirectorComponent::Get(this);
	if (!LootTable || !Director)
		return;
	FSoulDropPlacement Placement;
	Placement.ScatterRadius = ScatterRadius;
	Placement.Spacing = DropSpacing;
	Placement.NavExtent = DropNavExtent;
	Placement.FallbackForward = DropFallbackForward;
	LootTable->SpawnLoot(GetWorld(), Location, Director->GetCurrentFloor(), Placement);
}
