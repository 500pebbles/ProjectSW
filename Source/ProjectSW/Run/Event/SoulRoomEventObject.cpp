#include "Run/Event/SoulRoomEventObject.h"
#include "Run/Core/SoulRunSubsystem.h"
#include "Run/Core/SoulRunTags.h"
#include "Run/Flow/SoulFloorDirectorComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Run/Interact/SoulInteractableComponent.h"
#include "Run/Interact/SoulHighlightComponent.h"
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

	Highlight = CreateDefaultSubobject<USoulHighlightComponent>(TEXT("Highlight"));
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

	LockUser(User);
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
	if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this))
	{
		Run->AddEventUsed();
		if (ScoreReward > 0)
		{
			Run->AddScore(TEXT("Event"), ScoreReward);
		}
	}
	SW_LOG("[EVENT] %s applied magnitude=%.2f exp=%d score=%d", *GetClass()->GetName(), Magnitude, ExpReward, ScoreReward);
	if (USoulFloorDirectorComponent* Director = USoulFloorDirectorComponent::Get(this))
	{
		Director->NotifyRoomEventUsed(this);
	}

	GetWorldTimerManager().SetTimer(RecoverTimer, this, &ThisClass::Recover, FMath::Max(RecoverDelay, 0.01f), false);
}

void ASoulRoomEventObject::Recover()
{
	UnlockUser();
	ActiveInstigator.Reset();
}

void ASoulRoomEventObject::LockUser(APawn* User)
{
	APlayerController* PC = User ? Cast<APlayerController>(User->GetController()) : nullptr;
	if (!PC || bUserLocked)
		return;
	// 이동, 시점, 입력을 막고 하던 공격과 스킬을 끊는다
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
	PC->DisableInput(PC);
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(User))
	{
		ASC->CancelAllAbilities();
	}
	bUserLocked = true;
}

void ASoulRoomEventObject::UnlockUser()
{
	if (!bUserLocked)
		return;
	bUserLocked = false;
	APawn* User = ActiveInstigator.Get();
	APlayerController* PC = User ? Cast<APlayerController>(User->GetController()) : nullptr;
	if (!PC)
		return;
	PC->SetIgnoreMoveInput(false);
	PC->SetIgnoreLookInput(false);
	// Director가 잠근 상태(층 전환, 클리어, 사망)는 Director가 푼다
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(User);
	if (!ASC || !ASC->HasMatchingGameplayTag(SoulRunTags::State_Locked))
	{
		PC->EnableInput(PC);
	}
}

void ASoulRoomEventObject::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnlockUser();
	Super::EndPlay(EndPlayReason);
}

void ASoulRoomEventObject::ApplyEvent_Implementation(APawn* User)
{
}
