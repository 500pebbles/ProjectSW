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
#include "Run/UI/SoulTownWidgets.h"
#include "Run/Equipment/SoulEquipmentCatalog.h"
#include "Run/Equipment/SoulEquipmentDefinition.h"
#include "Run/Interact/SoulInteractionComponent.h"
#include "Animation/AnimSequence.h"
#include "Utility/SoulLog.h"
#include "Run/Core/SoulShots.h"
#include "GameFramework/Character.h"

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
#include "Blueprint/WidgetBlueprintLibrary.h"

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

	Card = CreateDefaultSubobject<UWidgetComponent>(TEXT("Card"));
	Card->SetupAttachment(Root);
	Card->SetWidgetSpace(EWidgetSpace::Screen);
	Card->SetDrawAtDesiredSize(true);
	Card->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Card->SetRelativeLocation(FVector(0.f, 0.f, 230.f));
	Card->SetHiddenInGame(true);
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
	if (PoseOverride)
	{
		Body->OverrideAnimationData(PoseOverride, false, false, PoseOverrideTime, 0.f);
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
	if (CardClass && Costume)
	{
		Card->SetWidgetClass(CardClass);
		Card->InitWidget();
		if (USoulCostumeCardWidget* Widget = Cast<USoulCostumeCardWidget>(Card->GetUserWidgetObject()))
		{
			Widget->SetCostume(Costume);
		}
		GetWorldTimerManager().SetTimer(CardTimer, this, &ThisClass::UpdateCardFocus, CardCheckInterval, true);
	}
}

void ASoulCostumeStatue::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(CardTimer);
	Super::EndPlay(EndPlayReason);
}

void ASoulCostumeStatue::UpdateCardFocus()
{
	const USoulInteractionComponent* Interaction = USoulInteractionComponent::Find(UGameplayStatics::GetPlayerPawn(this, 0));
	ShowCard(Interaction && Interaction->GetTraceTarget() == this);
}

void ASoulCostumeStatue::ShowCard(bool bShow)
{
	if (bShow == bCardShown || !Card->GetUserWidgetObject())
		return;
	bCardShown = bShow;
	if (bShow)
	{
		// 기록이 바뀌었을 수 있으니 보일 때마다 다시 채운다
		if (USoulCostumeCardWidget* Widget = Cast<USoulCostumeCardWidget>(Card->GetUserWidgetObject()))
		{
			Widget->SetCostume(Costume);
		}
	}
	Card->SetHiddenInGame(!bShow);
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
	const bool bSelected = bAlwaysSelected || (Costume && Run && Run->GetSelectedCostume() == Costume->GetFName());
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

// ---------- 1위 석상 ----------

ASoulChampionStatue::ASoulChampionStatue()
{
	NamePlate = CreateDefaultSubobject<UWidgetComponent>(TEXT("NamePlate"));
	NamePlate->SetupAttachment(Root);
	NamePlate->SetWidgetSpace(EWidgetSpace::Screen);
	NamePlate->SetDrawAtDesiredSize(true);
	NamePlate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NamePlate->SetRelativeLocation(FVector(0.f, 0.f, 240.f));
	NamePlate->SetWidgetClass(USoulNamePlateWidget::StaticClass());
	bAlwaysSelected = true;
}

void ASoulChampionStatue::BeginPlay()
{
	Super::BeginPlay();
	// 1위 석상은 입을 수 없다 (보기만)
	Interactable->SetInteractEnabled(false);
	if (USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this))
	{
		HoF->OnChanged.AddDynamic(this, &ThisClass::RefreshChampion);
	}
	RefreshChampion();
}

void ASoulChampionStatue::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this))
	{
		HoF->OnChanged.RemoveDynamic(this, &ThisClass::RefreshChampion);
	}
	Super::EndPlay(EndPlayReason);
}

void ASoulChampionStatue::RefreshChampion()
{
	const USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this);
	const TArray<FSoulHallOfFameEntry> Top = HoF ? HoF->GetTopEntries(1) : TArray<FSoulHallOfFameEntry>();
	const USoulEquipmentCatalog* Catalog = USoulEquipmentCatalog::GetRunCatalog(this);
	USoulCostumeDefinition* TopCostume = !Top.IsEmpty() && Catalog ? Catalog->FindCostumeByName(Top[0].CostumeId.ToString()) : nullptr;
	if (!TopCostume && Catalog && !Top.IsEmpty())
	{
		TopCostume = Catalog->DefaultCostume;
	}
	// 기록이 없으면 숨는다
	SetActorHiddenInGame(!TopCostume);
	// 숨긴 받침대가 보이지 않는 벽이 되지 않게
	SetActorEnableCollision(TopCostume != nullptr);
	if (!TopCostume)
	{
		SW_LOG("[TOWN] champion hidden (no record) collision=%d", GetActorEnableCollision());
		return;
	}
	const FSoulHallOfFameEntry& Entry = Top[0];
	Costume = TopCostume;
	PoseOverride = ChampionPose;
	PoseOverrideTime = ChampionPoseTime;
	ApplyCostumeVisuals();
	// 무기는 그 기록의 무기 (없으면 의상 석상 무기)
	if (const USoulEquipmentDefinition* TopWeapon = Catalog && !Entry.WeaponId.IsNone() ? Catalog->FindWeaponByName(Entry.WeaponId.ToString()) : nullptr)
	{
		Weapon->SetSkeletalMeshAsset(TopWeapon->WeaponMesh);
		if (TopWeapon->SheathMesh)
		{
			Saya->SetStaticMesh(TopWeapon->SheathMesh);
		}
	}
	RefreshTint(true);
	FText PlateText = FText::FromString(Entry.PlayerName);
	if (!NameFormat.IsEmpty())
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Name"), FText::FromString(Entry.PlayerName));
		Args.Add(TEXT("Score"), FText::AsNumber(Entry.Score));
		Args.Add(TEXT("Grade"), FText::FromName(Entry.RankId));
		PlateText = FText::Format(NameFormat, Args);
	}
	NamePlate->InitWidget();
	if (USoulNamePlateWidget* Plate = Cast<USoulNamePlateWidget>(NamePlate->GetUserWidgetObject()))
	{
		Plate->SetPlateText(PlateText);
	}
	SW_LOG("[TOWN] champion %s costume=%s weapon=%s pose=%s plate=%s", *Entry.PlayerName, *Costume->GetName(), *Entry.WeaponId.ToString(),
		ChampionPose ? *ChampionPose->GetName() : TEXT("none"), *PlateText.ToString());
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
	Interactable = CreateDefaultSubobject<USoulInteractableComponent>(TEXT("Interactable"));
	Interactable->SetupAttachment(Root);
}

void ASoulHallOfFameBoard::BeginPlay()
{
	Super::BeginPlay();
	if (USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this))
	{
		HoF->OnChanged.AddDynamic(this, &ThisClass::Refresh);
	}
	Interactable->OnInteracted.AddDynamic(this, &ThisClass::HandleInteracted);
	Interactable->SetInteractEnabled(PanelClass != nullptr);
	Refresh();
}

void ASoulHallOfFameBoard::HandleInteracted(APawn* User)
{
	USoulHallOfFamePanelWidget::Open(User ? Cast<APlayerController>(User->GetController()) : nullptr, PanelClass);
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
	if (USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this); Pres && Pres->GetData())
	{
		Pres->PlayBgm(Pres->GetData()->TownBgm);
	}
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
	if (SoulShots::IsEnabled())
	{
		RunTownShots();
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
			Statues[4]->ShowCard(true);
			Statues[4]->ShowCard(false);
		}
		for (TActorIterator<ASoulHallOfFameBoard> It(GetWorld()); It; ++It)
		{
			if (USoulInteractableComponent* BoardInteract = It->FindComponentByClass<USoulInteractableComponent>(); BoardInteract && BoardInteract->Interact(Pawn))
			{
				TArray<UUserWidget*> Panels;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Panels, USoulHallOfFamePanelWidget::StaticClass(), false);
				SW_LOG("[SELFTEST] town panel open=%d", Panels.Num());
				for (UUserWidget* Panel : Panels)
				{
					Cast<USoulHallOfFamePanelWidget>(Panel)->SelectEntry(0);
					Cast<USoulHallOfFamePanelWidget>(Panel)->Close();
				}
			}
			break;
		}
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

void ASoulTownDirector::RunTownShots()
{
#if !UE_BUILD_SHIPPING
	// 사람 확인용 장면: 마을 전경, 석상과 카드, 1위 석상, 명예의 전당 패널, 게이트(로딩 F1)
	auto Face = [this](const AActor* Target, float Distance)
	{
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		const ACharacter* Character = Cast<ACharacter>(Pawn);
		if (!Pawn || !Target)
			return;
		const FVector Dir = Target->GetActorForwardVector().GetSafeNormal2D();
		FVector Stand = Target->GetActorLocation() + Dir * Distance;
		Stand.Z = Target->GetActorLocation().Z + (Character ? Character->GetDefaultHalfHeight() : 90.f) + 10.f;
		const FRotator Look = (-Dir).Rotation();
		Pawn->TeleportTo(Stand, Look, false, true);
		if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			PC->SetControlRotation(FRotator(-10.f, Look.Yaw, 0.f));
		}
	};
	SoulShots::Take(TEXT("02_town"), 1.5f);
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Face]()
	{
		TArray<ASoulCostumeStatue*> Statues;
		for (TActorIterator<ASoulCostumeStatue> It(GetWorld()); It; ++It)
		{
			if (!Cast<ASoulChampionStatue>(*It))
			{
				Statues.Add(*It);
			}
		}
		Statues.Sort([](const ASoulCostumeStatue& A, const ASoulCostumeStatue& B) { return A.GetActorLocation().X > B.GetActorLocation().X; });
		if (Statues.Num() > 3)
		{
			Face(Statues[3], 320.f);
			Statues[3]->ShowCard(true);
		}
		SoulShots::Take(TEXT("03_statue_card"), 0.9f);
	}), 2.5f, false);
	FTimerHandle Handle2;
	GetWorldTimerManager().SetTimer(Handle2, FTimerDelegate::CreateWeakLambda(this, [this, Face]()
	{
		for (TActorIterator<ASoulChampionStatue> It(GetWorld()); It; ++It)
		{
			Face(*It, 420.f);
			break;
		}
		SoulShots::Take(TEXT("04_champion"), 0.9f);
	}), 4.5f, false);
	FTimerHandle Handle3;
	GetWorldTimerManager().SetTimer(Handle3, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		for (TActorIterator<ASoulHallOfFameBoard> It(GetWorld()); It; ++It)
		{
			if (USoulInteractableComponent* BoardInteract = It->FindComponentByClass<USoulInteractableComponent>())
			{
				BoardInteract->Interact(Pawn);
			}
			break;
		}
		SoulShots::Take(TEXT("05_hof_panel"), 0.9f);
	}), 6.5f, false);
	FTimerHandle Handle4;
	GetWorldTimerManager().SetTimer(Handle4, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		TArray<UUserWidget*> Panels;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Panels, USoulHallOfFamePanelWidget::StaticClass(), false);
		for (UUserWidget* Panel : Panels)
		{
			Cast<USoulHallOfFamePanelWidget>(Panel)->Close();
		}
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		for (TActorIterator<ASoulDungeonGate> It(GetWorld()); It; ++It)
		{
			if (USoulInteractableComponent* Interactable = It->FindComponentByClass<USoulInteractableComponent>())
			{
				SW_LOG("[SELFTEST] town gate (shots)");
				Interactable->Interact(Pawn);
				SoulShots::Take(TEXT("15_loading_f1"), 1.3f);
			}
			break;
		}
	}), 8.5f, false);
#endif
}

void ASoulTownDirector::OnTownArrived_Implementation(ESoulTownArrival Arrival)
{
	if (Arrival == ESoulTownArrival::FirstLaunch && TownHintClass)
	{
		if (USoulTownHintWidget* Hint = CreateWidget<USoulTownHintWidget>(UGameplayStatics::GetPlayerController(this, 0), TownHintClass))
		{
			Hint->AddToViewport(20);
		}
	}
	const USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this);
	SW_LOG("[TOWN] arrived=%d entries=%d place=%d best=%d", static_cast<int32>(Arrival), HoF ? HoF->GetEntryCount() : -1,
		HoF ? HoF->GetRankOf(HoF->GetLastSubmittedId()) : 0, HoF ? HoF->WasLastSubmittedNewBest() : 0);
}
