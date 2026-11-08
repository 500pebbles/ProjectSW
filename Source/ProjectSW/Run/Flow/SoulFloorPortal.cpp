#include "Run/Flow/SoulFloorPortal.h"
#include "Run/Flow/SoulFloorDirectorComponent.h"
#include "Character/SoulCharacterPlayer.h"
#include "Components/SphereComponent.h"
#include "Run/Interact/SoulInteractableComponent.h"
#include "Utility/SoulLog.h"

ASoulFloorPortal::ASoulFloorPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Trigger->SetGenerateOverlapEvents(true);
	SetRootComponent(Trigger);

	Interactable = CreateDefaultSubobject<USoulInteractableComponent>(TEXT("Interactable"));
	Interactable->SetupAttachment(Trigger);
	Interactable->bOneShot = true;
}

void ASoulFloorPortal::HandleInteracted(APawn* User)
{
	EnterPortal();
}

void ASoulFloorPortal::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleTriggerBeginOverlap);
	Interactable->OnInteracted.AddDynamic(this, &ThisClass::HandleInteracted);
}

void ASoulFloorPortal::EnterPortal()
{
	USoulFloorDirectorComponent* Director = USoulFloorDirectorComponent::Get(this);
	if (!Director)
	{
		SW_LOG_WARNING("ASoulFloorPortal::EnterPortal : Director 없음 (BP_DungeonGameMode가 아님)");
		return;
	}

	SW_LOG("[FLOOR] Portal entered: %s", *GetName());
	Director->RequestDescend();
}

void ASoulFloorPortal::HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bEnterOnOverlap && Cast<ASoulCharacterPlayer>(OtherActor))
	{
		EnterPortal();
	}
}
