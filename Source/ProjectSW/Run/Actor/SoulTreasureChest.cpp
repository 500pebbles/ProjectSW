#include "Run/Actor/SoulTreasureChest.h"
#include "Run/Core/SoulRunSubsystem.h"
#include "Run/Equipment/SoulLootDropComponent.h"
#include "Run/Interact/SoulInteractableComponent.h"
#include "Run/Interact/SoulHighlightComponent.h"
#include "Utility/SoulLog.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

ASoulTreasureChest::ASoulTreasureChest()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	Interactable = CreateDefaultSubobject<USoulInteractableComponent>(TEXT("Interactable"));
	Interactable->SetupAttachment(Mesh);
	Interactable->bOneShot = true;

	LootDrop = CreateDefaultSubobject<USoulLootDropComponent>(TEXT("LootDrop"));

	Highlight = CreateDefaultSubobject<USoulHighlightComponent>(TEXT("Highlight"));
}

void ASoulTreasureChest::BeginPlay()
{
	Super::BeginPlay();
	BaseScale = Mesh->GetRelativeScale3D();
	Interactable->OnInteracted.AddDynamic(this, &ThisClass::HandleInteracted);
}

void ASoulTreasureChest::HandleInteracted(APawn* User)
{
	Open(User);
}

void ASoulTreasureChest::Open(APawn* User)
{
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, OpenFX, GetActorLocation());
	UGameplayStatics::PlaySoundAtLocation(this, OpenSound, GetActorLocation());
	if (PulseScale > 0.f && PulseTime > 0.f)
	{
		Mesh->SetRelativeScale3D(BaseScale * PulseScale);
		GetWorldTimerManager().SetTimer(PulseTimer, this, &ThisClass::EndPulse, PulseTime, false);
	}

	LootDrop->DropLootAt(GetActorTransform().TransformPosition(DropOffset));
	if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this); Run && ScoreValue > 0)
	{
		Run->AddScore(TEXT("Chest"), ScoreValue);
	}
	SW_LOG("[CHEST] opened %s score=%d", *GetName(), ScoreValue);
	K2_OnOpened(User);
}

void ASoulTreasureChest::EndPulse()
{
	Mesh->SetRelativeScale3D(BaseScale);
}
