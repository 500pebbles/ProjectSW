#include "Run/Actor/SoulTownActors.h"
#include "Run/Core/SoulRunSubsystem.h"
#include "Run/Equipment/SoulCostumeDefinition.h"
#include "Run/Equipment/SoulEquipmentComponent.h"
#include "Run/Interact/SoulInteractableComponent.h"
#include "Run/Presentation/SoulTransitionSubsystem.h"
#include "Run/Presentation/SoulPresentationSubsystem.h"
#include "Run/Presentation/SoulPresentationData.h"
#include "Run/Save/SoulHallOfFameSubsystem.h"
#include "Run/UI/SoulHallOfFameBoardWidget.h"
#include "Utility/SoulLog.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

// ---------- 게이트 ----------

ASoulDungeonGate::ASoulDungeonGate()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	GateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateMesh"));
	GateMesh->SetupAttachment(Root);
	Interactable = CreateDefaultSubobject<USoulInteractableComponent>(TEXT("Interactable"));
	Interactable->SetupAttachment(Root);
	Interactable->bOneShot = true;
}

void ASoulDungeonGate::BeginPlay()
{
	Super::BeginPlay();
	Interactable->OnInteracted.AddDynamic(this, &ThisClass::HandleInteracted);
}

void ASoulDungeonGate::HandleInteracted(APawn* User)
{
	if (const USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this); Pres && Pres->GetData())
	{
		Pres->PlayAt(Pres->GetData()->EntrySound, Pres->GetData()->EntryFX, User ? User->GetActorLocation() : GetActorLocation());
		Pres->PlaySound2D(Pres->GetData()->EntryVoice);
	}
	// 클리어 시간은 게이트 F부터 (로딩과 연출 포함, 2.3)
	if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this))
	{
		Run->BeginRun();
	}
	if (USoulTransitionSubsystem* Transition = USoulTransitionSubsystem::Get(this))
	{
		Transition->TravelToDungeon(this);
	}
}

// ---------- 의상 석상 ----------

ASoulCostumeStatue::ASoulCostumeStatue()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Stand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stand"));
	Stand->SetupAttachment(Root);

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetComponentTickEnabled(false);
	Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon"));
	Weapon->SetupAttachment(Body);
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Weapon->SetComponentTickEnabled(false);

	Saya = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Saya"));
	Saya->SetupAttachment(Body);
	Saya->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Interactable = CreateDefaultSubobject<USoulInteractableComponent>(TEXT("Interactable"));
	Interactable->SetupAttachment(Root);
}

void ASoulCostumeStatue::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyCostumeVisuals();
}

void ASoulCostumeStatue::ApplyCostumeVisuals()
{
	if (!Costume)
		return;
	Body->SetSkeletalMeshAsset(Costume->BodyMesh);
	if (Costume->StatuePose)
	{
		// 포즈는 데이터: 재생하지 않고 한 시각에 세운다
		Body->OverrideAnimationData(Costume->StatuePose, false, false, Costume->StatuePoseTime, 0.f);
	}
	Weapon->SetSkeletalMeshAsset(Costume->StatueWeaponMesh);
	Saya->SetStaticMesh(Costume->StatueSheathMesh);
	// 소켓이 없는 몸 메시면 무기와 칼집을 숨긴다 (몸 원점에 떠 있지 않게)
	const bool bWeaponSocket = !WeaponSocketName.IsNone() && Body->DoesSocketExist(WeaponSocketName);
	const bool bSheathSocket = !SheathSocketName.IsNone() && Body->DoesSocketExist(SheathSocketName);
	if (bWeaponSocket)
	{
		Weapon->AttachToComponent(Body, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
	}
	if (bSheathSocket)
	{
		Saya->AttachToComponent(Body, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SheathSocketName);
	}
	Weapon->SetVisibility(bWeaponSocket);
	Saya->SetVisibility(bSheathSocket);
	if (!PromptFormat.IsEmpty())
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Name"), Costume->DisplayName);
		Interactable->PromptText = FText::Format(PromptFormat, Args);
	}
}

void ASoulCostumeStatue::BeginPlay()
{
	Super::BeginPlay();
	// 배치된 액터는 런타임에 OnConstruction이 다시 돌지 않으므로 여기서도 맞춘다
	ApplyCostumeVisuals();
	Interactable->OnInteracted.AddDynamic(this, &ThisClass::HandleInteracted);
	RefreshTint(true);
}

void ASoulCostumeStatue::HandleInteracted(APawn* User)
{
	if (USoulEquipmentComponent* Equipment = USoulEquipmentComponent::Find(User); Equipment && Costume)
	{
		Equipment->EquipCostume(Costume);
	}
}

void ASoulCostumeStatue::RefreshTint(bool bInstant)
{
	const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
	const bool bSelected = Costume && Run && Run->GetSelectedCostume() == Costume->GetFName();
	TargetTint = bSelected ? SelectedTint : UnselectedTint;
	if (bInstant || TintBlendTime <= 0.f)
	{
		ApplyTint(TargetTint);
		SetActorTickEnabled(false);
		return;
	}
	FromTint = CurrentTint;
	BlendElapsed = 0.f;
	SetActorTickEnabled(true);
}

void ASoulCostumeStatue::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	BlendElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(BlendElapsed / FMath::Max(TintBlendTime, 0.01f), 0.f, 1.f);
	ApplyTint(FMath::Lerp(FromTint, TargetTint, Alpha));
	if (Alpha >= 1.f)
	{
		SetActorTickEnabled(false);
	}
}

void ASoulCostumeStatue::ApplyTint(const FLinearColor& Color)
{
	CurrentTint = Color;
	if (TintParameterName.IsNone())
		return;
	Body->SetColorParameterValueOnMaterials(TintParameterName, Color);
	Weapon->SetColorParameterValueOnMaterials(TintParameterName, Color);
}

void ASoulCostumeStatue::RefreshAll(UWorld* World)
{
	if (!World)
		return;
	for (TActorIterator<ASoulCostumeStatue> It(World); It; ++It)
	{
		It->RefreshTint(false);
	}
}

// ---------- 명예의 전당 보드 ----------

ASoulHallOfFameBoard::ASoulHallOfFameBoard()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Sign = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sign"));
	Sign->SetupAttachment(Root);
	Board = CreateDefaultSubobject<UWidgetComponent>(TEXT("Board"));
	Board->SetupAttachment(Root);
	Board->SetWidgetSpace(EWidgetSpace::World);
	Board->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASoulHallOfFameBoard::BeginPlay()
{
	Super::BeginPlay();
	if (USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this))
	{
		HoF->OnChanged.AddDynamic(this, &ThisClass::Refresh);
	}
	Refresh();
}

void ASoulHallOfFameBoard::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this))
	{
		HoF->OnChanged.RemoveDynamic(this, &ThisClass::Refresh);
	}
	Super::EndPlay(EndPlayReason);
}

void ASoulHallOfFameBoard::Refresh()
{
	if (USoulHallOfFameBoardWidget* Widget = Cast<USoulHallOfFameBoardWidget>(Board->GetUserWidgetObject()))
	{
		Widget->Refresh();
	}
}

// ---------- 마을 도착 ----------

ASoulTownDirector::ASoulTownDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASoulTownDirector::BeginPlay()
{
	Super::BeginPlay();
	const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
	const USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this);
	ESoulTownArrival Arrival = ESoulTownArrival::FirstLaunch;
	if (Run && Run->GetState().bActive && Run->GetState().bRunEnded)
	{
		Arrival = Run->GetLastSummary().bSuccess ? ESoulTownArrival::ClearReturn : ESoulTownArrival::FailReturn;
	}
	else if (HoF && (HoF->GetEntryCount() > 0 || !HoF->GetLastCostumeId().IsNone()))
	{
		Arrival = ESoulTownArrival::Relaunch;
	}
	OnTownArrived(Arrival);
	if (USoulTransitionSubsystem* Transition = USoulTransitionSubsystem::Get(this))
	{
		Transition->HideLoading();
	}

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("SoulTownSelfTest")))
	{
		RunTownSelfTest(Arrival);
	}
#endif
}

void ASoulTownDirector::RunTownSelfTest(ESoulTownArrival Arrival)
{
#if !UE_BUILD_SHIPPING
	SW_LOG("[SELFTEST] town arrival=%d", static_cast<int32>(Arrival));
	if (Arrival == ESoulTownArrival::ClearReturn || Arrival == ESoulTownArrival::FailReturn)
	{
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this))
			{
				HoF->DumpToLog();
			}
			APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
			const USoulEquipmentComponent* Equipment = USoulEquipmentComponent::Find(Pawn);
			const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
			SW_LOG("[SELFTEST] town return costume=%s inputEnabled=%d cursor=%d", Equipment ? *Equipment->GetCurrentCostumeId().ToString() : TEXT("none"),
				Pawn ? Pawn->InputEnabled() : -1, PC ? PC->bShowMouseCursor : -1);
			SW_LOG("[SELFTEST] DONE town");
			UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
		}), 2.f, false);
		return;
	}
	// 첫 도착: 두 번째 석상의 의상을 입고 게이트로
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		TArray<ASoulCostumeStatue*> Statues;
		for (TActorIterator<ASoulCostumeStatue> It(GetWorld()); It; ++It)
		{
			Statues.Add(*It);
		}
		Statues.Sort([](const ASoulCostumeStatue& A, const ASoulCostumeStatue& B) { return A.GetActorLocation().X > B.GetActorLocation().X; });
		SW_LOG("[SELFTEST] town statues=%d", Statues.Num());
		if (Statues.Num() > 4)
		{
			if (USoulInteractableComponent* Interactable = Statues[4]->FindComponentByClass<USoulInteractableComponent>())
			{
				Interactable->Interact(Pawn);
			}
		}
		FTimerHandle Next;
		GetWorldTimerManager().SetTimer(Next, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
			if (const UCharacterMovementComponent* Move = Pawn ? Pawn->FindComponentByClass<UCharacterMovementComponent>() : nullptr)
			{
				SW_LOG("[SELFTEST] town walkSpeed=%.0f", Move->MaxWalkSpeed);
			}
			for (TActorIterator<ASoulDungeonGate> It(GetWorld()); It; ++It)
			{
				if (USoulInteractableComponent* Interactable = It->FindComponentByClass<USoulInteractableComponent>())
				{
					SW_LOG("[SELFTEST] town gate");
					Interactable->Interact(Pawn);
				}
				break;
			}
		}), 1.5f, false);
	}), 2.f, false);
#endif
}

void ASoulTownDirector::OnTownArrived_Implementation(ESoulTownArrival Arrival)
{
	const USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this);
	SW_LOG("[TOWN] arrived=%d entries=%d place=%d best=%d", static_cast<int32>(Arrival), HoF ? HoF->GetEntryCount() : -1,
		HoF ? HoF->GetRankOf(HoF->GetLastSubmittedId()) : 0, HoF ? HoF->WasLastSubmittedNewBest() : 0);
}
