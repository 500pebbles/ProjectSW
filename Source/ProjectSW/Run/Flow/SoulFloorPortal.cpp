#include "Run/Flow/SoulFloorPortal.h"
#include "Run/Flow/SoulFloorDirectorComponent.h"
#include "Character/SoulCharacterPlayer.h"
#include "Components/SphereComponent.h"
#include "Run/Interact/SoulInteractableComponent.h"
#include "Run/Interact/SoulHighlightComponent.h"
#include "Run/UI/SoulKeyHintWidget.h"
#include "Components/WidgetComponent.h"
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

	Highlight = CreateDefaultSubobject<USoulHighlightComponent>(TEXT("Highlight"));

	Marker = CreateDefaultSubobject<UWidgetComponent>(TEXT("Marker"));
	Marker->SetupAttachment(Trigger);
	Marker->SetWidgetSpace(EWidgetSpace::Screen);
	Marker->SetDrawAtDesiredSize(true);
	Marker->SetWidgetClass(USoulKeyHintWidget::StaticClass());
	Marker->SetRelativeLocation(FVector(0.f, 0.f, 260.f));
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Marker->SetHiddenInGame(true);
}

void ASoulFloorPortal::ShowMarker(const FText& InText, const FLinearColor& InColor)
{
	Marker->InitWidget();
	if (USoulKeyHintWidget* Widget = Cast<USoulKeyHintWidget>(Marker->GetUserWidgetObject()))
	{
		Widget->SetHint(InText, InColor);
	}
	Marker->SetHiddenInGame(false);
	SW_LOG("[FLOOR] portal marker \"%s\" widget=%d", *InText.ToString(), Marker->GetUserWidgetObject() != nullptr);
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
	Marker->SetHiddenInGame(true);
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
