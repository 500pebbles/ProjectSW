#include "Run/Event/SoulRoomEventObject.h"
#include "Run/Core/SoulRunSubsystem.h"
#include "Run/Interact/SoulInteractableComponent.h"
#include "Run/Progression/SoulProgressionComponent.h"
#include "Utility/SoulLog.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

ASoulRoomEventObject::ASoulRoomEventObject()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(Root);

	TopMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopMesh"));
	TopMesh->SetupAttachment(Root);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Root);
	Glow->SetCastShadows(false);

	Interactable = CreateDefaultSubobject<USoulInteractableComponent>(TEXT("Interactable"));
	Interactable->SetupAttachment(Root);
	Interactable->bOneShot = true;
}

void ASoulRoomEventObject::BeginPlay()
{
	Super::BeginPlay();
	Interactable->OnInteracted.AddDynamic(this, &ThisClass::HandleInteracted);
}

void ASoulRoomEventObject::Configure(float InMagnitude, int32 InExpReward, int32 InScoreReward)
{
	Magnitude = InMagnitude;
	ExpReward = InExpReward;
	ScoreReward = InScoreReward;
}

void ASoulRoomEventObject::ForceActivate(APawn* User)
{
	if (Interactable->bEnabled)
	{
		Interactable->Interact(User);
	}
}

void ASoulRoomEventObject::HandleInteracted(APawn* User)
{
	if (State != ESoulRoomEventState::Idle || !User)
		return;

	State = ESoulRoomEventState::Charging;
	ActiveInstigator = User;

	if (APlayerController* PC = Cast<APlayerController>(User->GetController()))
	{
		User->DisableInput(PC);
	}
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, IgniteFX, GetActorLocation());
	UGameplayStatics::PlaySoundAtLocation(this, IgniteSound, GetActorLocation());

	GetWorldTimerManager().SetTimer(ChargeTimer, this, &ThisClass::FinishCharge, FMath::Max(ChargeTime, 0.01f), false);
}

void ASoulRoomEventObject::FinishCharge()
{
	APawn* User = ActiveInstigator.Get();
	State = ESoulRoomEventState::Spent;
	if (!User)
		return;

	ApplyEvent(User);
	K2_OnApplied(User);

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ResultFX, GetActorLocation());
	UGameplayStatics::PlaySoundAtLocation(this, ResultSound, GetActorLocation());
	Glow->SetVisibility(false);

	if (USoulProgressionComponent* Progression = USoulProgressionComponent::Find(User); Progression && ExpReward > 0)
	{
		Progression->AddExp(ExpReward, TEXT("Event"));
	}
	if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this); Run && ScoreReward > 0)
	{
		Run->AddScore(TEXT("Event"), ScoreReward);
	}
	SW_LOG("[EVENT] %s applied magnitude=%.2f exp=%d score=%d", *GetClass()->GetName(), Magnitude, ExpReward, ScoreReward);

	GetWorldTimerManager().SetTimer(RecoverTimer, this, &ThisClass::Recover, FMath::Max(RecoverDelay, 0.01f), false);
}

void ASoulRoomEventObject::Recover()
{
	APawn* User = ActiveInstigator.Get();
	if (User)
	{
		if (APlayerController* PC = Cast<APlayerController>(User->GetController()))
		{
			User->EnableInput(PC);
		}
	}
	ActiveInstigator.Reset();
}

void ASoulRoomEventObject::ApplyEvent_Implementation(APawn* User)
{
}
