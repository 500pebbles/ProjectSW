#include "Run/Flow/SoulFloorDirectorComponent.h"
#include "Run/Flow/SoulDungeonFlowData.h"
#include "Run/Flow/SoulFloorPortal.h"
#include "Run/Core/SoulRunSubsystem.h"
#include "Run/Core/SoulRunGameComponent.h"
#include "Run/Core/SoulRunConfig.h"
#include "Run/Core/SoulRunTags.h"
#include "Run/Progression/SoulProgressionComponent.h"
#include "Run/Interact/SoulInteractionComponent.h"
#include "Run/Interact/SoulInteractableComponent.h"
#include "Run/Interact/SoulItemLibrary.h"
#include "Run/Presentation/SoulTransitionSubsystem.h"
#include "Run/Presentation/SoulPresentationSubsystem.h"
#include "Run/Presentation/SoulPresentationData.h"
#include "Run/Save/SoulHallOfFameSubsystem.h"
#include "Run/UI/SoulResultWidget.h"
#include "Run/Equipment/SoulEquipmentComponent.h"
#include "Run/Equipment/SoulEquipmentDefinition.h"
#include "Run/Equipment/SoulEquipmentCatalog.h"
#include "Run/Event/SoulRoomEventObject.h"
#include "Run/Event/SoulRoomEvent_Effect.h"
#include "Run/Event/SoulFloorMine.h"
#include "Run/Actor/SoulTreasureChest.h"
#include "Run/Mob/SoulSpawnPatternData.h"
#include "GAS/Core/SoulAbilitySystemComponent.h"
#include "Run/Mob/SoulMobComponent.h"
#include "Run/Mob/SoulMobEncounterSubsystem.h"
#include "Actor/SoulProjectileBase.h"
#include "Character/SoulCharacterBase.h"
#include "Character/SoulCharacterEnemy.h"
#include "Interface/SoulCombatInterface.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Map/SoulPackedLevelActor.h"
#include "Map/SoulMapSubsystem.h"
#include "Map/SoulMapWidget.h"
#include "Run/Interact/SoulHighlightComponent.h"
#include "Player/SoulPlayerController.h"
#include "UI/HUD/SoulHUD.h"
#include "Utility/SoulGameplayTags.h"
#include "Utility/SoulLog.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "Input/SoulInputConfig.h"
#include "InputKeyEventArgs.h"
#include "Run/Combat/SoulCombatFeelSubsystem.h"
#include "Run/UI/SoulPresentationWidgets.h"
#include "Run/UI/SoulKeyHintWidget.h"
#include "Build/PMGMapBuilder.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BTNode.h"
#include "Inventory/Player/InventoryComponent.h"
#include "Item/ItemActorBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/PMGSettings.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "UObject/UObjectIterator.h"

namespace SoulFloorDirector
{
	/** soul.Floor.SelfTest가 Director보다 먼저 들어왔을 때 기억해 둔다 */
	static bool bSelfTestRequested = false;

	/** 셀프 테스트에서 층이 준비된 뒤 다음 단계까지 (디버그 전용) */
	static constexpr float SelfTestStepDelaySec = 3.f;

	/** 셀프 테스트 한 단계의 제한 시간. 넘으면 FAIL (디버그 전용) */
	static constexpr float SelfTestStepTimeoutSec = 60.f;
}

namespace SoulFloorDirectorDebug
{
	/** 디버그: 살아 있는 적을 모두 죽인다. 팀 Die 경로(OnDied → 방 전투 종료, 몹 사망 몽타주)를 그대로 탄다 */
	static void KillAllMobs(UWorld* World)
	{
		TArray<ASoulCharacterEnemy*> Targets;
		for (TActorIterator<ASoulCharacterEnemy> It(World); It; ++It)
		{
			if (IsValid(*It) && !ISoulCombatInterface::Execute_IsDead(*It))
			{
				Targets.Add(*It);
			}
		}
		for (ASoulCharacterEnemy* Enemy : Targets)
		{
			Enemy->Die();
		}
		SW_LOG("[SELFTEST] KillAll count=%d", Targets.Num());
	}

	/** 디버그: 살아 있는 보스 체력을 최대 체력 x Ratio만큼 깎는다 (소환 임계 확인용, 0 아래로는 안 내림) */
	static ASoulCharacterEnemy* DamageBoss(UWorld* World, float Ratio)
	{
		for (TActorIterator<ASoulCharacterEnemy> It(World); It; ++It)
		{
			const USoulMobComponent* Mob = USoulMobComponent::Find(*It);
			UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(*It);
			if (!Mob || Mob->GetRole() != ESoulMobRole::Boss || !ASC || ISoulCombatInterface::Execute_IsDead(*It))
				continue;
			const float Max = ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute());
			const float Health = ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute());
			ASC->SetNumericAttributeBase(USoulAttributeSet::GetHealthAttribute(), FMath::Max(1.f, Health - Max * Ratio));
			SW_LOG("[SELFTEST] DamageBoss %s hp %.0f -> %.0f (%.0f%%)", *It->GetName(), Health, ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute()),
				100.f * ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute()) / FMath::Max(Max, 1.f));
			return *It;
		}
		SW_LOG_WARNING("[SELFTEST] DamageBoss: 살아 있는 보스 없음");
		return nullptr;
	}

	static int32 CountSummons(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<ASoulCharacterEnemy> It(World); It; ++It)
		{
			const USoulMobComponent* Mob = USoulMobComponent::Find(*It);
			if (Mob && Mob->GetRole() == ESoulMobRole::Summon && !It->IsActorBeingDestroyed() && !ISoulCombatInterface::Execute_IsDead(*It))
			{
				++Count;
			}
		}
		return Count;
	}
}

USoulFloorDirectorComponent::USoulFloorDirectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

USoulFloorDirectorComponent* USoulFloorDirectorComponent::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	return GameMode ? GameMode->FindComponentByClass<USoulFloorDirectorComponent>() : nullptr;
}

// ---------- 시작 ----------

void USoulFloorDirectorComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority())
		return;

	// 첫 층이 준비될 때까지 입력과 화면을 막는다 (폰은 아직 없다. 팀 GM이 빌더가 있으면 폰을 만들지 않는다)
	if (APlayerController* PC = GetPlayerController())
	{
		PC->DisableInput(PC);
	}
	CurtainOut(0.f);
	CurtainClosedTime = GetWorld()->GetTimeSeconds();

#if !UE_BUILD_SHIPPING
	// 콘솔로 켠 셀프 테스트는 이번 한 번만 쓴다 (이후 PIE가 계속 자동 진행되지 않게)
	if (SoulFloorDirector::bSelfTestRequested || FParse::Param(FCommandLine::Get(), TEXT("SoulFloorSelfTest")))
	{
		bSelfTest = true;
		SW_LOG("[SELFTEST] 켜짐");
		ArmSelfTestWatchdog(TEXT("first floor"));
	}
	SoulFloorDirector::bSelfTestRequested = false;
#endif

	// 컴포넌트 BeginPlay는 GM의 ReceiveBeginPlay보다 먼저라, 팀 GM의 바인딩은 다음 틱에 뗀다
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::StartFirstFloor);
}

void USoulFloorDirectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideMapKeyHint();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	if (Builder.IsValid())
	{
		Builder->OnGenerationFinished.RemoveDynamic(this, &ThisClass::HandleGenerationFinished);
	}

	Super::EndPlay(EndPlayReason);
}

void USoulFloorDirectorComponent::StartFirstFloor()
{
	TActorIterator<APMGMapBuilder> It(GetWorld());
	Builder = It ? *It : nullptr;

	// Director에 따로 넣지 않았으면 DA_RunConfig의 흐름을 쓴다
	if (!FlowData)
	{
		if (const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(this))
		{
			FlowData = Config->DungeonFlow;
		}
	}

	if (!Builder.IsValid() || !FlowData || FlowData->Floors.IsEmpty())
	{
		SW_LOG_ERROR("[FLOOR] 시작 불가: Builder=%d FlowData=%d Floors=%d", Builder.IsValid(), FlowData != nullptr, FlowData ? FlowData->Floors.Num() : 0);
		AbortFloorStart(TEXT("setup"));
		return;
	}

	USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
	if (Run && !Run->IsRunActive())
	{
		// L_Dungeon 직접 실행: 런 없이 들어오면 1층부터
		Run->BeginRun();
	}

	// 팀 GM의 OnMapReady가 재생성마다 폰을 다시 만들고 HUD를 중복시키므로 첫 Generate 전에 뗀다
	const AGameModeBase* GameMode = Cast<AGameModeBase>(GetOwner());
	const bool bBoundBefore = Builder->OnGenerationFinished.Contains(GameMode, TeamMapReadyFunction);
	Builder->OnGenerationFinished.Remove(GameMode, TeamMapReadyFunction);
	const bool bBoundAfter = Builder->OnGenerationFinished.Contains(GameMode, TeamMapReadyFunction);
	SW_LOG("[FLOOR] Team OnMapReady binding before=%d after=%d", bBoundBefore, bBoundAfter);
	if (!bBoundBefore)
	{
		SW_LOG_WARNING("[FLOOR] 팀 GameMode의 %s 바인딩을 찾지 못함. 팀 GM이 바뀌었으면 TeamMapReadyFunction을 고친다", *TeamMapReadyFunction.ToString());
	}

	Builder->OnGenerationFinished.AddUniqueDynamic(this, &ThisClass::HandleGenerationFinished);

	const int32 RunFloor = Run ? Run->GetState().Floor : 1;
	bFirstFloor = true;
	bTransitioning = true;
	BeginFloor(FMath::Clamp(RunFloor - 1, 0, FlowData->Floors.Num() - 1));
}

void USoulFloorDirectorComponent::BeginFloor(int32 FloorIndex)
{
	if (bPlayerDead)
		return;

	CurrentFloorIndex = FloorIndex;
	GenerateAttempt = 0;
	bBossIntroDone = false;

	if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this))
	{
		Run->SetFloor(FloorIndex + 1);
	}

	GenerateCurrentFloor();
}

void USoulFloorDirectorComponent::GenerateCurrentFloor()
{
	UPMGSettings* Settings = FlowData->Floors[CurrentFloorIndex].MapSettings;
	if (!Settings)
	{
		SW_LOG_ERROR("[FLOOR] %d층 MapSettings가 비어 있음 (DA_DungeonFlow)", CurrentFloorIndex + 1);
		if (bFirstFloor)
		{
			AbortFloorStart(TEXT("map settings"));
		}
		else
		{
			// 이전 층을 지운 빈 월드라 폰을 풀면 떨어진다. 생성 실패와 같은 경로로 보낸다
			if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this))
			{
				Run->EndRun(false);
			}
			ShowRunResult();
		}
		return;
	}

	Builder->Settings = Settings;
	if (FlowData->SpawnTimeBudgetMs > 0.f)
	{
		Builder->SpawnTimeBudgetMs = FlowData->SpawnTimeBudgetMs;
	}

	const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
	const int32 Seed = (Run ? Run->GetState().RunSeed : 0) + CurrentFloorIndex + 1 + GenerateAttempt;

	SW_LOG("[FLOOR] Generate floor=%d seed=%d attempt=%d settings=%s", CurrentFloorIndex + 1, Seed, GenerateAttempt, *Settings->GetName());
	GenerateStartTime = FPlatformTime::Seconds();
	bNavReady = false;
	LastGeneratedSeed = Seed;
	Builder->Generate(Seed);
}

void USoulFloorDirectorComponent::HandleGenerationFinished(const FPMGResult& Result, const TArray<FString>& SpawnErrors)
{
	int32 ConnectedRooms = 0;
	for (const FPMGResultRoom& Room : Result.Rooms)
	{
		ConnectedRooms += Room.bConnected ? 1 : 0;
	}

	SW_LOG("[FLOOR] Generated floor=%d success=%d attempts=%d rooms=%d spawnErrors=%d sec=%.2f",
		CurrentFloorIndex + 1, Result.bSuccess, Result.AttemptCount, ConnectedRooms, SpawnErrors.Num(), FPlatformTime::Seconds() - GenerateStartTime);

	for (int32 i = 0; i < FMath::Min(SpawnErrors.Num(), 10); ++i)
	{
		SW_LOG_WARNING("[FLOOR] SpawnError: %s", *SpawnErrors[i]);
	}

	if (!Result.bSuccess)
	{
		if (GenerateAttempt < FlowData->GenerateRetryCount)
		{
			++GenerateAttempt;
			SW_LOG_WARNING("[FLOOR] 생성 실패, 재시도 %d: %s", GenerateAttempt, *Result.FailReason);
			GenerateCurrentFloor();
			return;
		}

		// 런 실패로 끝내고 결과창 → 마을
		SW_LOG_ERROR("[FLOOR] 생성 실패, 재시도 소진: %s", *Result.FailReason);
		if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this))
		{
			Run->EndRun(false);
		}
		ShowRunResult();
		return;
	}

	PlacePawn(Result);
	LastResult = Result;
	if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this))
	{
		Run->NotifyFloorGenerated(LastGeneratedSeed);
	}
	// 층 몹 배치: 방의 저작 포인트를 지우고 층 세트의 포인트를 만든다 (실제 스폰은 방 입장 때)
	if (USoulMobEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<USoulMobEncounterSubsystem>())
	{
		const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
		const int32 Seed = (Run ? Run->GetState().RunSeed : 0) * 31 + CurrentFloorIndex + 1;
		Encounter->BeginFloor(FlowData->Floors[CurrentFloorIndex].MobRules, Result, CurrentFloorIndex + 1, Seed, Builder->Settings->CellSize, Builder->GetActorLocation());
	}

	// 이벤트 오브젝트, 상자, 지뢰 (폰은 아직 공중에 세워져 있다)
	SpawnFloorObjects(Result);
	SpawnFloorMines(Result);

	ExitRoom = FindRoomActor(Result.ExitRoomId);
	if (ExitRoom.IsValid())
	{
		ExitRoom->OnCombatFinished.AddUObject(this, &ThisClass::HandleExitCombatFinished);
		ExitRoom->OnPlayerEntered.AddUObject(this, &ThisClass::HandleExitEntered);
	}
	else
	{
		SW_LOG_WARNING("[FLOOR] Exit 방 액터를 찾지 못함 (id=%d)", Result.ExitRoomId);
	}

	LogFloorState(TEXT("Generated"));

	// 텔레메트리: 방 입장과 전투 종료 시각 (SW.Run.Telemetry 1)
	RoomEnterTime.Reset();
	for (TActorIterator<ASoulPackedLevelActor> RoomIt(GetWorld()); RoomIt; ++RoomIt)
	{
		RoomIt->OnPlayerEntered.AddUObject(this, &ThisClass::HandleRoomEnteredTelemetry);
		RoomIt->OnCombatFinished.AddUObject(this, &ThisClass::HandleRoomCombatFinishedTelemetry);
	}

	NavWaitStartTime = GetWorld()->GetTimeSeconds();
	// 방 스폰이 만든 dirty area가 큐에 들어간 다음 프레임부터 본다
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::PollNavigation);
}

void USoulFloorDirectorComponent::PlacePawn(const FPMGResult& Result)
{
	const ASoulPackedLevelActor* EntranceRoom = FindRoomActor(Result.EntranceRoomId);
	const FVector Center = EntranceRoom ? EntranceRoom->GetActorLocation() : Builder->GetActorLocation();
	const FRotator Rotation(0.f, EntranceRoom ? EntranceRoom->GetActorRotation().Yaw : 0.f, 0.f);
	const FVector Location = Center + FlowData->ArrivalOffset + FVector(0.f, 0.f, FlowData->ArrivalDropHeight);

	if (!EntranceRoom)
	{
		SW_LOG_WARNING("[FLOOR] 입구 방 액터를 찾지 못함 (id=%d). 빌더 위치에 놓음", Result.EntranceRoomId);
	}

	APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		SW_LOG_ERROR("[FLOOR] PlayerController 없음");
		return;
	}

	APawn* Pawn = PlayerPawn.Get();
	if (!Pawn)
	{
		// 첫 층: 팀 GM이 폰을 만들지 않았으므로 여기서 스폰한다 (빙의 1회)
		if (AGameModeBase* GameMode = Cast<AGameModeBase>(GetOwner()))
		{
			GameMode->RestartPlayerAtTransform(PC, FTransform(Rotation, Location));
		}
		Pawn = PC->GetPawn();
		PlayerPawn = Pawn;

		if (ASoulCharacterBase* Character = Cast<ASoulCharacterBase>(Pawn))
		{
			Character->OnDied.AddUObject(this, &ThisClass::HandlePawnDied);
		}
		if (Pawn)
		{
			Pawn->DisableInput(PC);
		}
		if (Pawn)
		{
			SW_LOG("[FLOOR] Pawn spawned: %s", *Pawn->GetName());
		}
		else
		{
			SW_LOG_ERROR("[FLOOR] 폰 스폰 실패 (GameMode의 DefaultPawnClass를 확인)");
		}
	}
	else
	{
		Pawn->TeleportTo(Location, Rotation, false, true);
		PC->SetControlRotation(Rotation);
	}

	// 커튼이 걷힐 때까지 공중에 세워 둔다. 충돌도 꺼서 방 트리거가 먼저 반응하지 않게 한다
	ParkPawn();
}

void USoulFloorDirectorComponent::PollNavigation()
{
	const float Waited = GetWorld()->GetTimeSeconds() - NavWaitStartTime;
	// 빌드 중이거나 아직 큐에 dirty area가 있으면 기다린다
	const bool bBuilding = UNavigationSystemV1::IsNavigationBeingBuilt(GetWorld());

	if (bBuilding && Waited < FlowData->NavWaitTimeoutSec)
	{
		GetWorld()->GetTimerManager().SetTimer(NavTimer, this, &ThisClass::PollNavigation, FMath::Max(FlowData->NavPollIntervalSec, 0.01f), false);
		return;
	}

	SW_LOG("[FLOOR] NavMesh wait=%.2fs building=%d", Waited, bBuilding);
	bNavReady = true;
	TryReveal();
}

void USoulFloorDirectorComponent::TryReveal()
{
	const float Held = GetWorld()->GetTimeSeconds() - CurtainClosedTime;
	if (Held < CurtainMinHoldSec)
	{
		GetWorld()->GetTimerManager().SetTimer(RevealTimer, this, &ThisClass::TryReveal, CurtainMinHoldSec - Held, false);
		return;
	}

	Reveal();
}

void USoulFloorDirectorComponent::Reveal()
{
	if (bPlayerDead)
		return;

	UnparkPawn();
	CurtainIn(CurtainFadeInSec);
	UnlockPlayer();
	// 받은 피해 집계는 첫 입력 복구 뒤부터 (의상 체력 감소와 초기화가 섞이지 않게)
	if (!bDamageTracked)
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerPawn.Get()))
		{
			ASC->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetHealthAttribute()).AddUObject(this, &ThisClass::HandlePlayerHealthChanged);
			bDamageTracked = true;
		}
	}
	ApplyDescendHeal();
	if (USoulTransitionSubsystem* Transition = USoulTransitionSubsystem::Get(this))
	{
		Transition->HideLoading();
	}
	if (const USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this); Pres && Pres->GetData())
	{
		if (const FSoulLoadingEntry* Entry = Pres->GetData()->FindLoading(GetLoadingId(CurrentFloorIndex)))
		{
			Pres->ShowBanner(Entry->Title, Entry->Subtitle, Entry->Color);
		}
		if (PlayerPawn.IsValid())
		{
			Pres->PlayAt(Pres->GetData()->LandSound, Pres->GetData()->LandFX, PlayerPawn->GetActorLocation() - FVector(0.f, 0.f, PlayerPawn->GetDefaultHalfHeight()));
		}
	}

	bTransitioning = false;
	bFirstFloor = false;
	ShowMapKeyHint();
	if (USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this); Pres && Pres->GetData())
	{
		Pres->PlayBgm(Pres->GetData()->DungeonBgm);
	}

	LogFloorState(TEXT("Ready"));
	FloorReadyTime = GetWorld()->GetTimeSeconds();
	LogTelemetry(TEXT("floor_ready"));
	OnFloorReady.Broadcast(CurrentFloorIndex + 1);

	// Exit 방에 전투가 없으면 클리어 신호가 오지 않으므로 바로 포탈을 예약한다
	if (ExitRoom.IsValid() && !ExitRoom->HasCombat() && IsLastFloor())
	{
		// 보스 세트나 보스 포인트가 빠지면 클리어 신호가 오지 않아 런이 멈춘다
		SW_LOG_ERROR("[FLOOR] 마지막 층 Exit 방에 보스 포인트 없음 (MobRules.BossSet, BossRoomType 확인), 바로 클리어");
		BeginClear();
	}
	else if (ExitRoom.IsValid() && !ExitRoom->HasCombat() && !IsLastFloor())
	{
		SW_LOG("[FLOOR] Exit 방에 전투 없음, 포탈 예약");
		SchedulePortal();
	}

	if (bSelfTest)
	{
		GetWorld()->GetTimerManager().SetTimer(SelfTestTimer, this, &ThisClass::RunSelfTestStep, SoulFloorDirector::SelfTestStepDelaySec, false);
	}
}

// ---------- 층 이동 ----------

void USoulFloorDirectorComponent::RequestDescend()
{
	if (bTransitioning || bClearPending || bPlayerDead || CurrentFloorIndex == INDEX_NONE)
	{
		SW_LOG("[FLOOR] RequestDescend 무시: transitioning=%d clear=%d dead=%d floor=%d", bTransitioning, bClearPending, bPlayerDead, CurrentFloorIndex + 1);
		return;
	}
	if (IsLastFloor())
	{
		SW_LOG("[FLOOR] RequestDescend 무시: 마지막 층 (%d)", CurrentFloorIndex + 1);
		return;
	}

	SW_LOG("[FLOOR] Descend %d -> %d", CurrentFloorIndex + 1, CurrentFloorIndex + 2);
	LogTelemetry(FString::Printf(TEXT("floor_end floorSec=%.1f"), GetWorld()->GetTimeSeconds() - FloorReadyTime));
	bTransitioning = true;
	bDescendHealPending = true;
	if (bSelfTest)
	{
		ArmSelfTestWatchdog(TEXT("descend"));
	}
	GetWorld()->GetTimerManager().ClearTimer(PortalTimer);

	OnDescendStarted.Broadcast(CurrentFloorIndex + 1);

	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	LockPlayer();
	CurtainOut(CurtainFadeOutSec);
	if (USoulTransitionSubsystem* Transition = USoulTransitionSubsystem::Get(this))
	{
		Transition->ShowLoading(GetLoadingId(CurrentFloorIndex + 1));
	}
	if (const USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this); Pres && Pres->GetData() && PlayerPawn.IsValid())
	{
		Pres->PlayAt(Pres->GetData()->FloorMoveSound, Pres->GetData()->FloorMoveFX, PlayerPawn->GetActorLocation());
		Pres->PlaySound2D(Pres->GetData()->FloorMoveVoice);
	}

	GetWorld()->GetTimerManager().SetTimer(CurtainTimer, this, &ThisClass::UnloadFloor, FMath::Max(CurtainFadeOutSec, 0.01f), false);
}

void USoulFloorDirectorComponent::UnloadFloor()
{
	CurtainClosedTime = GetWorld()->GetTimeSeconds();

	ParkPawn();
	ExitRoom.Reset();
	if (USoulMobEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<USoulMobEncounterSubsystem>())
	{
		Encounter->EndFloor();
	}
	DestroyFloorActors();
	Builder->Clear();
	UKismetSystemLibrary::CollectGarbage();

	SW_LOG("[FLOOR] Unloaded floor=%d", CurrentFloorIndex + 1);

	const int32 NextIndex = CurrentFloorIndex + 1;
	FTimerDelegate Next = FTimerDelegate::CreateUObject(this, &ThisClass::BeginFloor, NextIndex);
	GetWorld()->GetTimerManager().SetTimer(UnloadTimer, Next, FMath::Max(FlowData->UnloadHoldSec, 0.01f), false);
}

void USoulFloorDirectorComponent::DestroyFloorActors()
{
	// 빌더가 지우지 않는 것: 적과 시체, 투사체, 등록한 액터 (포탈 등). 방이 지워지기 전에 지워야 방의 정리 콜백이 안전하다
	if (USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this))
	{
		Pres->ClearBoss();
	}
	TArray<AActor*> ToDestroy;
	for (TActorIterator<ASoulCharacterEnemy> It(GetWorld()); It; ++It)
	{
		ToDestroy.Add(*It);
	}
	const int32 EnemyCount = ToDestroy.Num();
	for (TActorIterator<ASoulProjectileBase> It(GetWorld()); It; ++It)
	{
		ToDestroy.Add(*It);
	}
	// 인벤토리에서 끌어 버린 아이템은 등록되지 않으므로 따로 (Director는 런 맵에만 있다)
	for (TActorIterator<AItemActorBase> It(GetWorld()); It; ++It)
	{
		ToDestroy.AddUnique(*It);
	}
	for (const TWeakObjectPtr<AActor>& Actor : FloorActors)
	{
		if (Actor.IsValid())
		{
			ToDestroy.AddUnique(Actor.Get());
		}
	}
	FloorActors.Reset();
	FloorEventObjects.Reset();
	bEventSetBonusGiven = false;
	Portal.Reset();

	for (AActor* Actor : ToDestroy)
	{
		Actor->Destroy();
	}

	SW_LOG("[FLOOR] Destroyed floor actors=%d (enemies=%d)", ToDestroy.Num(), EnemyCount);
}

void USoulFloorDirectorComponent::RegisterFloorActor(AActor* Actor)
{
	if (IsValid(Actor))
	{
		FloorActors.AddUnique(Actor);
	}
}

// ---------- Exit 방, 포탈, 클리어 ----------

void USoulFloorDirectorComponent::HandleExitEntered(ASoulPackedLevelActor* Room)
{
	// 인트로는 층마다 한 번 (문 쪽으로 물러났다 다시 들어와도 다시 돌지 않는다)
	if (bTransitioning || bPlayerDead || bBossIntroDone || Room != ExitRoom.Get())
		return;
	// 보스 스폰은 같은 입장에서 일어나므로 조금 뒤에 찾는다
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (bBossIntroDone || bPlayerDead)
			return;
		for (TActorIterator<ASoulCharacterEnemy> It(GetWorld()); It; ++It)
		{
			const USoulMobComponent* Mob = USoulMobComponent::Find(*It);
			if (Mob && Mob->GetRole() == ESoulMobRole::Boss && !ISoulCombatInterface::Execute_IsDead(*It))
			{
				bBossIntroDone = true;
				SW_LOG("[FLOOR] boss intro floor=%d boss=%s", CurrentFloorIndex + 1, *It->GetName());
				if (USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this))
				{
					Pres->PlayBossIntro(*It);
				}
				return;
			}
		}
	}), 0.2f, false);
}

void USoulFloorDirectorComponent::HandleExitCombatFinished(ASoulPackedLevelActor* Room)
{
	if (bTransitioning || bClearPending || bPlayerDead || Room != ExitRoom.Get())
		return;

	SW_LOG("[FLOOR] Exit 방 전투 종료 floor=%d", CurrentFloorIndex + 1);
	if (USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this))
	{
		Pres->StopBgm();
	}

	if (const USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this); Pres && Pres->GetData())
	{
		Pres->ShowBanner(IsLastFloor() ? Pres->GetData()->DungeonClearText : Pres->GetData()->FloorClearText, FText::GetEmpty(), Pres->GetData()->ClearBannerColor);
		Pres->PlaySound2D(Pres->GetData()->FloorClearSound);
	}
	if (IsLastFloor())
	{
		BeginClear();
	}
	else
	{
		SchedulePortal();
	}
}

static TAutoConsoleVariable<int32> CVarSoulRunTelemetry(TEXT("SW.Run.Telemetry"), 0, TEXT("1이면 방, 층, 보스 시간과 체력, 레벨을 [TEL] 로그로 찍는다"));

void USoulFloorDirectorComponent::LogTelemetry(const FString& Event) const
{
	if (CVarSoulRunTelemetry.GetValueOnGameThread() <= 0)
		return;
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<APawn*>(PlayerPawn.Get()));
	const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
	SW_LOG("[TEL] floor=%d t=%.1f run=%.1f %s hp=%.0f/%.0f lv=%d exp=%d", CurrentFloorIndex + 1, GetWorld()->GetTimeSeconds() - FloorReadyTime,
		Run ? Run->GetElapsedRealSeconds() : 0.0, *Event,
		ASC ? ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute()) : -1.f, ASC ? ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute()) : -1.f,
		Run ? Run->GetState().Level : 0, Run ? Run->GetState().Exp : 0);
}

void USoulFloorDirectorComponent::HandleRoomEnteredTelemetry(ASoulPackedLevelActor* Room)
{
	if (!Room || RoomEnterTime.Contains(Room->GetRoomId()))
		return;
	RoomEnterTime.Add(Room->GetRoomId(), GetWorld()->GetTimeSeconds());
	const FName Type = LastResult.Rooms.IsValidIndex(Room->GetRoomId()) ? LastResult.Rooms[Room->GetRoomId()].RoomType : NAME_None;
	LogTelemetry(FString::Printf(TEXT("room_enter id=%d type=%s"), Room->GetRoomId(), *Type.ToString()));
}

void USoulFloorDirectorComponent::HandleRoomCombatFinishedTelemetry(ASoulPackedLevelActor* Room)
{
	if (!Room || bTransitioning)
		return;
	const double* Enter = RoomEnterTime.Find(Room->GetRoomId());
	LogTelemetry(FString::Printf(TEXT("room_clear id=%d fightSec=%.1f boss=%d"), Room->GetRoomId(), Enter ? GetWorld()->GetTimeSeconds() - *Enter : -1.0, Room == ExitRoom.Get()));
}

void USoulFloorDirectorComponent::SchedulePortal()
{
	GetWorld()->GetTimerManager().SetTimer(PortalTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		SpawnPortal();
	}), FMath::Max(PortalDelaySec, 0.01f), false);
}

ASoulFloorPortal* USoulFloorDirectorComponent::SpawnPortal()
{
	if (Portal.IsValid())
		return Portal.Get();

	if (!PortalClass || !ExitRoom.IsValid())
	{
		SW_LOG_WARNING("[FLOOR] 포탈을 세울 수 없음: PortalClass=%d ExitRoom=%d", PortalClass != nullptr, ExitRoom.IsValid());
		return nullptr;
	}

	// Exit 방 중심에서 시작해 바닥 드랍과 PortalDropClearance 이상 떨어진 Nav 점 (보스 드랍을 주우러 가다 포탈에 닿지 않게)
	TArray<FVector> Drops;
	for (TActorIterator<AItemActorBase> It(GetWorld()); It; ++It)
	{
		Drops.Add(It->GetActorLocation());
	}
	const float Clearance = FlowData ? FlowData->PortalDropClearance : 0.f;
	const FVector Center = ExitRoom->GetActorLocation();
	FVector Location = Center;
	bool bFound = false;
	if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		const float Step = FMath::Max(Clearance, 100.f);
		for (int32 Ring = 0; Ring <= 4 && !bFound; ++Ring)
		{
			const int32 Count = Ring == 0 ? 1 : 8;
			for (int32 i = 0; i < Count && !bFound; ++i)
			{
				const float Angle = 2.f * PI * i / Count;
				const FVector Probe = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Step * Ring;
				FNavLocation NavLocation;
				if (!NavSys->ProjectPointToNavigation(Probe, NavLocation))
					continue;
				const bool bNearDrop = Drops.ContainsByPredicate([&](const FVector& Drop) { return FVector::Dist2D(Drop, NavLocation.Location) < Clearance; });
				if (!bNearDrop)
				{
					Location = NavLocation.Location;
					bFound = true;
				}
			}
		}
	}
	if (!bFound)
	{
		SW_LOG_WARNING("[FLOOR] 포탈 자리: 드랍과 %.0fcm 떨어진 Nav 점이 없어 방 중심에 세움 drops=%d", Clearance, Drops.Num());
	}
	float Nearest = -1.f;
	for (const FVector& Drop : Drops)
	{
		const float Distance = FVector::Dist2D(Drop, Location);
		Nearest = Nearest < 0.f ? Distance : FMath::Min(Nearest, Distance);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASoulFloorPortal* NewPortal = GetWorld()->SpawnActor<ASoulFloorPortal>(PortalClass, Location, FRotator::ZeroRotator, Params);
	if (!NewPortal)
	{
		SW_LOG_ERROR("[FLOOR] 포탈 스폰 실패 floor=%d", CurrentFloorIndex + 1);
		return nullptr;
	}

	Portal = NewPortal;
	RegisterFloorActor(NewPortal);
	if (const USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this); Pres && Pres->GetData())
	{
		Pres->PlayAt(Pres->GetData()->PortalSound, Pres->GetData()->PortalFX, Location);
	}
	ShowPortalGuide(NewPortal);
	SW_LOG("[FLOOR] Portal spawned floor=%d at %s nearestDrop=%.0f drops=%d", CurrentFloorIndex + 1, *Location.ToCompactString(), Nearest, Drops.Num());
	return NewPortal;
}

void USoulFloorDirectorComponent::ShowPortalGuide(ASoulFloorPortal* InPortal) const
{
	const USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this);
	const USoulPresentationData* Data = Pres ? Pres->GetData() : nullptr;
	if (!Data || !InPortal)
		return;

	const USoulInteractionComponent* Interaction = USoulInteractionComponent::Find(PlayerPawn.Get());
	FFormatNamedArguments Args;
	Args.Add(TEXT("Key"), USoulKeyHintWidget::GetActionKeyText(GetPlayerController(), Interaction ? Interaction->GetInteractAction() : nullptr, INVTEXT("F")));
	if (!Data->PortalOpenText.IsEmpty())
	{
		Pres->ShowBanner(Data->PortalOpenText, FText::Format(Data->PortalOpenSubtitleFormat, Args), Data->PortalBannerColor);
	}
	if (!Data->PortalMarkerFormat.IsEmpty())
	{
		InPortal->ShowMarker(FText::Format(Data->PortalMarkerFormat, Args), Data->PortalBannerColor);
	}
	SW_LOG("[FLOOR] portal guide banner=\"%s / %s\"", *Data->PortalOpenText.ToString(), *FText::Format(Data->PortalOpenSubtitleFormat, Args).ToString());
}

const UInputAction* USoulFloorDirectorComponent::GetMapAction() const
{
	// 팀 PC의 MapAction은 비공개라 이름으로 읽는다
	const APlayerController* PC = GetPlayerController();
	const FObjectProperty* Property = PC ? FindFProperty<FObjectProperty>(PC->GetClass(), TEXT("MapAction")) : nullptr;
	return Property ? Cast<UInputAction>(Property->GetObjectPropertyValue_InContainer(PC)) : nullptr;
}

void USoulFloorDirectorComponent::ShowMapKeyHint()
{
	const USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this);
	const USoulPresentationData* Data = Pres ? Pres->GetData() : nullptr;
	APlayerController* PC = GetPlayerController();
	if (!Data || !PC || Data->MapKeyHintFormat.IsEmpty())
		return;
	if (!MapKeyHint)
	{
		MapKeyHint = CreateWidget<USoulKeyHintWidget>(PC, USoulKeyHintWidget::StaticClass());
		if (!MapKeyHint)
			return;
		MapKeyHint->FontSize = 16;
	}
	FFormatNamedArguments Args;
	const FText Key = USoulKeyHintWidget::GetActionKeyText(PC, GetMapAction(), INVTEXT("M"));
	Args.Add(TEXT("Key"), Key);
	if (!MapKeyHint->IsInViewport())
	{
		MapKeyHint->AddToViewport(5);
	}
	MapKeyHint->SetHint(FText::Format(Data->MapKeyHintFormat, Args), Data->MapKeyHintColor);
	MapKeyHint->SetAnchorsInViewport(FAnchors(Data->MapKeyHintAnchor.X, Data->MapKeyHintAnchor.Y));
	MapKeyHint->SetAlignmentInViewport(Data->MapKeyHintAnchor);
	MapKeyHint->SetPositionInViewport(Data->MapKeyHintOffset, false);
	SW_LOG("[FLOOR] map hint key=%s floor=%d", *Key.ToString(), CurrentFloorIndex + 1);
}

void USoulFloorDirectorComponent::HideMapKeyHint()
{
	if (MapKeyHint)
	{
		MapKeyHint->RemoveFromParent();
	}
}

void USoulFloorDirectorComponent::DebugForceClear()
{
	if (!bClearPending && !bPlayerDead)
	{
		BeginClear();
	}
}

void USoulFloorDirectorComponent::BeginClear()
{
	bClearPending = true;
	SW_LOG("[FLOOR] Clear pending floor=%d", CurrentFloorIndex + 1);

	LockPlayer();
	for (TActorIterator<ASoulPackedLevelActor> It(GetWorld()); It; ++It)
	{
		It->SetTriggerEnabled(false);
	}

	// 클리어 확정 뒤에는 남은 탄이나 소환물에 죽지 않는다 (플랜 2.3)
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerPawn.Get()))
	{
		ASC->AddLooseGameplayTag(SoulTags::States::Invincible);
		ASC->AddLooseGameplayTag(SoulRunTags::State_Cleared);
	}

	// 몹 OnDied 뒤쪽 구독자(점수, 경험치)가 같은 스택에서 끝난 다음에 정산한다
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this))
		{
			Run->EndRun(true);
		}
		GetWorld()->GetTimerManager().SetTimer(RestartTimer, this, &ThisClass::ShowRunResult, FMath::Max(ClearRestartDelaySec, 0.01f), false);
	}));
}

// ---------- 사망 ----------

void USoulFloorDirectorComponent::HandlePawnDied(AActor* DeadActor)
{
	if (bClearPending || bPlayerDead)
		return;

	bPlayerDead = true;
	if (USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this))
	{
		Pres->StopBgm();
	}
	SW_LOG("[FLOOR] Player died floor=%d", CurrentFloorIndex + 1);

	// 전환 중에 죽으면 다음 층이 생성되지 않게 전환 타이머를 멈춘다
	GetWorld()->GetTimerManager().ClearTimer(PortalTimer);
	GetWorld()->GetTimerManager().ClearTimer(CurtainTimer);
	GetWorld()->GetTimerManager().ClearTimer(UnloadTimer);
	// 보스 인트로 중이면 멈춘다 (입력은 사망 잠금 그대로)
	if (USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this))
	{
		Pres->AbortIntroForDeath();
	}
	LockPlayer();

	if (ACharacter* Character = Cast<ACharacter>(DeadActor))
	{
		// 팀 코드가 메시 물리를 켜고 캡슐 충돌을 끄므로, 캡슐이 떨어지지 않게 이동을 끄고 물리를 되돌린다
		Character->GetCharacterMovement()->DisableMovement();
		Character->GetMesh()->SetSimulatePhysics(false);

		UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
		if (DeathMontage && AnimInstance)
		{
			AnimInstance->Montage_Play(DeathMontage);
		}
	}

	GetWorld()->GetTimerManager().SetTimer(DeathTimer, this, &ThisClass::FinishDeath, FMath::Max(DeathEndRunDelaySec, 0.01f), false);
	FTimerHandle BannerHandle;
	GetWorld()->GetTimerManager().SetTimer(BannerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (const USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this); Pres && Pres->GetData())
		{
			Pres->ShowBanner(Pres->GetData()->FailedText, FText::GetEmpty(), Pres->GetData()->FailedBannerColor);
		}
	}), FMath::Max(USoulPresentationSubsystem::Get(this) && USoulPresentationSubsystem::Get(this)->GetData() ? USoulPresentationSubsystem::Get(this)->GetData()->FailedBannerDelaySec : 0.f, 0.01f), false);
}

void USoulFloorDirectorComponent::FinishDeath()
{
	if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this))
	{
		Run->EndRun(false);
	}

	ShowRunResult();
}

void USoulFloorDirectorComponent::HandlePlayerHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue < Data.OldValue)
	{
		if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this))
		{
			Run->AddDamageTaken(Data.OldValue - Data.NewValue);
		}
	}
}

void USoulFloorDirectorComponent::ShowRunResult()
{
	SW_LOG("[FLOOR] Run result floor=%d", CurrentFloorIndex + 1);
	HideMapKeyHint();
	// 생성 실패 경로에서 로딩 화면이 남지 않게 최소 표시 시간을 무시하고 내린다
	if (USoulTransitionSubsystem* Transition = USoulTransitionSubsystem::Get(this))
	{
		Transition->HideLoading(true);
	}
#if !UE_BUILD_SHIPPING
	if (bSelfTest && SelfTestMode != TEXT("result"))
	{
		// 셀프 테스트는 재시작 요청까지만 확인하고 끝낸다 (다시 열면 테스트가 처음부터 돈다)
		SW_LOG("[SELFTEST] restart requested");
		if (SelfTestMode == TEXT("death"))
		{
			FinishSelfTest();
		}
		else
		{
			SW_LOG_ERROR("[SELFTEST] FAIL restart requested mode=%s floor=%d", *SelfTestMode, CurrentFloorIndex + 1);
			GLog->Flush();
			FPlatformMisc::RequestExitWithStatus(true, 3);
		}
		return;
	}
#endif
	const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(this);
	const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
	const USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this);
	APlayerController* PC = GetPlayerController();
	if (Config && Config->ResultWidgetClass && PC && Run)
	{
		USoulTransitionSubsystem::PrepareForTravel(PC);
		ResultWidget = USoulResultWidget::ShowResult(PC, Config->ResultWidgetClass, Run->GetLastSummary(), HoF ? HoF->GetLastSubmittedId() : FGuid());
#if !UE_BUILD_SHIPPING
		if (bSelfTest && ResultWidget)
		{
			// 결과창을 2초 보여 주고 이름을 넣어 귀환 (마을의 TownDirector 자체 확인이 이어 받는다)
			GetWorld()->GetTimerManager().SetTimer(SelfTestTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				GetWorld()->GetTimerManager().ClearTimer(SelfTestWatchdog);
				if (ResultWidget)
				{
					ResultWidget->SetNameForTest(TEXT("  자동확인테스터이름길다  "));
					ResultWidget->ReturnToTown();
				}
			}), 2.f, false);
		}
#endif
		return;
	}
	// 결과창이 없으면 바로 마을로
	if (USoulTransitionSubsystem* Transition = USoulTransitionSubsystem::Get(this))
	{
		Transition->TravelToTown(this);
	}
}

// ---------- 입력, 폰, 커튼 ----------

void USoulFloorDirectorComponent::LockPlayer()
{
	APlayerController* PC = GetPlayerController();
	if (!PC)
		return;

	if (const ASoulHUD* HUD = Cast<ASoulHUD>(PC->GetHUD()); HUD && HUD->IsMapOpen())
	{
		if (ASoulPlayerController* SoulPC = Cast<ASoulPlayerController>(PC))
		{
			SoulPC->ToggleMap();
		}
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PC->GetPawn()))
	{
		ASC->CancelAllAbilities();
	}
	// 인벤토리와 장비창도 닫는다 (플랜 2.3 공통 규칙)
	if (USoulInteractionComponent* Interaction = USoulInteractionComponent::Find(PC->GetPawn()))
	{
		Interaction->CloseInventoryUI();
	}

	PC->FlushPressedKeys();

	// FlushPressedKeys가 만든 Released가 다음 프레임 입력 처리에서 돌아야 질주 해제 같은 처리가 된다.
	// 이번 프레임 타이머 틱 전에 건 next-tick은 같은 프레임에 돌 수 있어 프레임 번호로 한 번 더 미룬다
	DisableInputAfterFrame(PC, GFrameCounter);

	if (APawn* Pawn = PC->GetPawn())
	{
		if (UAbilitySystemComponent* PawnASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
		{
			PawnASC->AddLooseGameplayTag(SoulRunTags::State_Locked);
		}
	}
}

void USoulFloorDirectorComponent::DisableInputAfterFrame(TWeakObjectPtr<APlayerController> WeakPC, uint64 LockFrame)
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, WeakPC, LockFrame]()
	{
		APlayerController* LockedPC = WeakPC.Get();
		if (!LockedPC)
			return;
		if (GFrameCounter <= LockFrame + 1)
		{
			DisableInputAfterFrame(WeakPC, LockFrame);
			return;
		}
		LockedPC->DisableInput(LockedPC);
		if (APawn* Pawn = LockedPC->GetPawn())
		{
			Pawn->DisableInput(LockedPC);
		}
	}));
}

void USoulFloorDirectorComponent::AbortFloorStart(const TCHAR* Reason)
{
	// 설정이 빠져 층을 못 열면 커튼과 입력을 풀어 화면이 검은 채로 멈추지 않게 한다
	SW_LOG_ERROR("[FLOOR] 층 시작 중단: %s", Reason);
	bTransitioning = false;
	if (USoulTransitionSubsystem* Transition = USoulTransitionSubsystem::Get(this))
	{
		Transition->HideLoading(true);
	}
	UnparkPawn();
	CurtainIn(0.f);
	UnlockPlayer();
}

void USoulFloorDirectorComponent::UnlockPlayer()
{
	APlayerController* PC = GetPlayerController();
	if (!PC)
		return;

	PC->EnableInput(PC);
	// 층 이동 뒤에도 의상 이동 배율과 대시 키 상태를 다시 맞춘다
	if (ASoulPlayerController* SoulPC = Cast<ASoulPlayerController>(PC))
	{
		SoulPC->RefreshMoveSpeed();
	}
	if (APawn* Pawn = PC->GetPawn())
	{
		Pawn->EnableInput(PC);
		if (UAbilitySystemComponent* PawnASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
		{
			PawnASC->SetLooseGameplayTagCount(SoulRunTags::State_Locked, 0);
		}
	}
}

void USoulFloorDirectorComponent::ParkPawn()
{
	ACharacter* Character = Cast<ACharacter>(PlayerPawn.Get());
	if (!Character)
		return;

	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->GetCharacterMovement()->DisableMovement();
	Character->SetActorEnableCollision(false);
}

void USoulFloorDirectorComponent::UnparkPawn()
{
	ACharacter* Character = Cast<ACharacter>(PlayerPawn.Get());
	if (!Character)
		return;

	Character->SetActorEnableCollision(true);
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
}

FName USoulFloorDirectorComponent::GetLoadingId(int32 FloorIndex) const
{
	return FName(*FString::Printf(TEXT("F%d"), FloorIndex + 1));
}

void USoulFloorDirectorComponent::CurtainOut(float Duration)
{
	// 임시 커튼: 카메라 페이드. 단계 F에서 WBP_RunLoadingScreen으로 바꾼다
	APlayerController* PC = GetPlayerController();
	if (!PC || !PC->PlayerCameraManager)
		return;

	if (Duration <= 0.f)
	{
		PC->PlayerCameraManager->SetManualCameraFade(1.f, FLinearColor::Black, false);
	}
	else
	{
		PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, Duration, FLinearColor::Black, false, true);
	}
}

void USoulFloorDirectorComponent::CurtainIn(float Duration)
{
	APlayerController* PC = GetPlayerController();
	if (!PC || !PC->PlayerCameraManager)
		return;

	if (Duration <= 0.f)
	{
		PC->PlayerCameraManager->StopCameraFade();
		PC->PlayerCameraManager->SetManualCameraFade(0.f, FLinearColor::Black, false);
	}
	else
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, Duration, FLinearColor::Black, false, false);
	}
}

void USoulFloorDirectorComponent::PrepareOpenLevel()
{
	// 입력 모드는 뷰포트에 남아 LoadMap을 넘으므로 이동 직전에 게임 모드로 돌린다
	if (APlayerController* PC = GetPlayerController())
	{
		if (const ASoulHUD* HUD = Cast<ASoulHUD>(PC->GetHUD()); HUD && HUD->IsMapOpen())
		{
			if (ASoulPlayerController* SoulPC = Cast<ASoulPlayerController>(PC))
			{
				SoulPC->ToggleMap();
			}
		}
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
}

// ---------- 도우미 ----------

APlayerController* USoulFloorDirectorComponent::GetPlayerController() const
{
	return GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
}

ASoulPackedLevelActor* USoulFloorDirectorComponent::FindRoomActor(int32 RoomId) const
{
	if (RoomId == INDEX_NONE)
		return nullptr;

	for (TActorIterator<ASoulPackedLevelActor> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && It->GetRoomId() == RoomId)
		{
			return *It;
		}
	}
	return nullptr;
}

float USoulFloorDirectorComponent::GetCellSize() const
{
	return Builder.IsValid() && Builder->Settings ? Builder->Settings->CellSize : 0.f;
}

FVector USoulFloorDirectorComponent::GetBuilderOrigin() const
{
	return Builder.IsValid() ? Builder->GetActorLocation() : FVector::ZeroVector;
}

int32 USoulFloorDirectorComponent::GetRoomIdAt(const FVector& Location) const
{
	const float CellSize = GetCellSize();
	if (CellSize <= 0.f)
		return INDEX_NONE;
	const FVector Origin = GetBuilderOrigin();
	const FIntPoint Cell(FMath::FloorToInt((Location.X - Origin.X) / CellSize), FMath::FloorToInt((Location.Y - Origin.Y) / CellSize));
	for (const FPMGResultRoom& Room : LastResult.Rooms)
	{
		if (Room.bConnected && Room.Bounds.Contains(Cell))
			return Room.Id;
	}
	return INDEX_NONE;
}

void USoulFloorDirectorComponent::SpawnFloorObjects(const FPMGResult& Result)
{
	const FSoulFloorDefinition& Floor = FlowData->Floors[CurrentFloorIndex];
	const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
	FRandomStream Stream((Run ? Run->GetState().RunSeed : 0) * 17 + CurrentFloorIndex + 1);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 이벤트: 보장 개수를 먼저 채우고 남는 방은 가중치. 방 순서는 층 시드로 섞는다
	TArray<int32> EventRooms;
	for (const FPMGResultRoom& Room : Result.Rooms)
	{
		if (Room.bConnected && !FlowData->EventRoomType.IsNone() && Room.RoomType == FlowData->EventRoomType)
			EventRooms.Add(Room.Id);
	}
	TArray<const FSoulEventRow*> Picks;
	for (const FSoulEventRow& Row : Floor.EventRows)
	{
		for (int32 i = 0; i < Row.GuaranteedCount; ++i)
			Picks.Add(&Row);
	}
	while (Picks.Num() < EventRooms.Num())
	{
		float Total = 0.f;
		for (const FSoulEventRow& Row : Floor.EventRows)
			Total += Row.Weight;
		if (Total <= 0.f)
			break;
		float Pick = Stream.FRandRange(0.f, Total);
		for (const FSoulEventRow& Row : Floor.EventRows)
		{
			Pick -= Row.Weight;
			if (Pick <= 0.f)
			{
				Picks.Add(&Row);
				break;
			}
		}
	}
	for (int32 i = Picks.Num() - 1; i > 0; --i)
	{
		Picks.Swap(i, Stream.RandRange(0, i));
	}
	if (!Floor.EventRows.IsEmpty() && EventRooms.IsEmpty())
	{
		SW_LOG_ERROR("[EVENT] floor=%d 이벤트 행은 있는데 %s 방이 0개", CurrentFloorIndex + 1, *FlowData->EventRoomType.ToString());
	}

	int32 Spawned = 0;
	for (int32 i = 0; i < FMath::Min(EventRooms.Num(), Picks.Num()); ++i)
	{
		const FSoulEventRow* Row = Picks[i];
		if (!Row->ObjectClass)
		{
			SW_LOG_WARNING("[EVENT] floor=%d room=%d ObjectClass가 빈 행", CurrentFloorIndex + 1, EventRooms[i]);
			continue;
		}
		const FTransform RoomTransform = PMG::GetRoomSpawnTransform(Result.Rooms[EventRooms[i]], GetCellSize(), GetBuilderOrigin());
		ASoulRoomEventObject* Object = GetWorld()->SpawnActor<ASoulRoomEventObject>(Row->ObjectClass, RoomTransform, Params);
		if (Object)
		{
			Object->Configure(Row->Magnitude, Row->ExpReward, Row->ScoreReward);
			RegisterFloorActor(Object);
			FloorEventObjects.Add(Object);
			++Spawned;
			SW_LOG("[EVENT] floor=%d room=%d %s", CurrentFloorIndex + 1, EventRooms[i], *Object->GetClass()->GetName());
		}
	}

	// 상자: Treasure 방, 중앙의 화로를 피해 로컬 오프셋
	int32 Chests = 0;
	if (!Floor.TreasureChestClass || FlowData->TreasureRoomType.IsNone())
	{
		SW_LOG_WARNING("[CHEST] floor=%d TreasureChestClass나 TreasureRoomType이 비어 상자 없음", CurrentFloorIndex + 1);
	}
	else
	{
		for (const FPMGResultRoom& Room : Result.Rooms)
		{
			if (!Room.bConnected || Room.RoomType != FlowData->TreasureRoomType)
				continue;
			const FTransform RoomTransform = PMG::GetRoomSpawnTransform(Room, GetCellSize(), GetBuilderOrigin());
			const FTransform ChestTransform(RoomTransform.GetRotation(), RoomTransform.TransformPosition(FlowData->TreasureChestLocal));
			if (ASoulTreasureChest* Chest = GetWorld()->SpawnActor<ASoulTreasureChest>(Floor.TreasureChestClass, ChestTransform, Params))
			{
				Chest->SetScoreValue(Floor.ChestScore);
				RegisterFloorActor(Chest);
				++Chests;
				SW_LOG("[CHEST] floor=%d room=%d", CurrentFloorIndex + 1, Room.Id);
			}
		}
	}
	SW_LOG("[CHEST] floor=%d chests=%d", CurrentFloorIndex + 1, Chests);
	SW_LOG("[EVENT] floor=%d events=%d/%d", CurrentFloorIndex + 1, Spawned, EventRooms.Num());
}

void USoulFloorDirectorComponent::SpawnFloorMines(const FPMGResult& Result)
{
	const FSoulFloorDefinition& Floor = FlowData->Floors[CurrentFloorIndex];
	if (Floor.MineRows.IsEmpty() || !FlowData->MinePatterns)
		return;

	const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
	FRandomStream Stream((Run ? Run->GetState().RunSeed : 0) * 29 + CurrentFloorIndex + 1);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 제비: 놓을 수 있는 방마다 칸 수만큼 (넓은 방에 많이)
	TArray<int32> Tickets;
	for (const FPMGResultRoom& Room : Result.Rooms)
	{
		if (!Room.bConnected || !FlowData->MineRoomTypes.Contains(Room.RoomType))
			continue;
		const int32 Cells = Room.Bounds.Area();
		for (int32 i = 0; i < Cells; ++i)
			Tickets.Add(Room.Id);
	}

	TArray<FVector> Placed;
	TArray<FVector> Objects;
	for (const TWeakObjectPtr<AActor>& Actor : FloorActors)
	{
		if (Actor.IsValid())
			Objects.Add(Actor->GetActorLocation());
	}

	for (const FSoulMineRow& Row : Floor.MineRows)
	{
		int32 Done = 0;
		int32 Rejected[4] = { 0, 0, 0, 0 };
		if (!Row.MineClass)
		{
			SW_LOG_WARNING("[MINE] floor=%d MineClass가 빈 행", CurrentFloorIndex + 1);
			continue;
		}
		for (int32 i = Tickets.Num() - 1; i > 0; --i)
		{
			Tickets.Swap(i, Stream.RandRange(0, i));
		}
		for (int32 t = 0; t < Tickets.Num() && Done < Row.Count; ++t)
		{
			const FPMGResultRoom& Room = Result.Rooms[Tickets[t]];
			const FTransform RoomTransform = PMG::GetRoomSpawnTransform(Room, GetCellSize(), GetBuilderOrigin());
			const TArray<FVector>* Candidates = &FlowData->MinePatterns->SingleRoom;
			if (Room.Shape == EPMGRoomShape::DualHorizontal || Room.Shape == EPMGRoomShape::DualVertical)
				Candidates = &FlowData->MinePatterns->DualRoom;
			else if (Room.Shape == EPMGRoomShape::Quad)
				Candidates = &FlowData->MinePatterns->QuadRoom;
			TArray<FVector> Pool = *Candidates;
			for (int32 i = Pool.Num() - 1; i > 0; --i)
			{
				Pool.Swap(i, Stream.RandRange(0, i));
			}
			const TArray<FTransform> Openings = PMG::GetRoomOpeningTransforms(Room.Shape, GetCellSize(), RoomTransform.GetLocation());

			for (const FVector& Local : Pool)
			{
				const FVector World = RoomTransform.TransformPosition(Local);
				const bool bNearDoor = Openings.ContainsByPredicate([&](const FTransform& O) { return FVector::Dist2D(O.GetLocation(), World) < FlowData->MineDoorClearance; });
				const bool bNearObject = Objects.ContainsByPredicate([&](const FVector& O) { return FVector::Dist2D(O, World) < FlowData->MineObjectClearance; });
				const bool bNearMine = Placed.ContainsByPredicate([&](const FVector& O) { return FVector::Dist2D(O, World) < FlowData->MineSpacing; });
				const bool bBlocked = FlowData->MineProbeRadius > 0.f && GetWorld()->OverlapBlockingTestByChannel(World + FVector(0.f, 0.f, FlowData->MineProbeHeight), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(FlowData->MineProbeRadius));
				Rejected[0] += bNearDoor; Rejected[1] += bNearObject; Rejected[2] += bNearMine; Rejected[3] += bBlocked;
				if (bNearDoor || bNearObject || bNearMine || bBlocked)
					continue;

				if (ASoulFloorMine* Mine = GetWorld()->SpawnActor<ASoulFloorMine>(Row.MineClass, FTransform(RoomTransform.GetRotation(), World), Params))
				{
					Mine->SetDamageRatio(Row.DamageRatio);
					RegisterFloorActor(Mine);
					Placed.Add(World);
					++Done;
				}
				break;
			}
		}
		SW_LOG("[MINE] floor=%d placed=%d/%d rooms=%d rejected door=%d object=%d mine=%d blocked=%d", CurrentFloorIndex + 1, Done, Row.Count, Tickets.Num(), Rejected[0], Rejected[1], Rejected[2], Rejected[3]);
		if (Done < Row.Count)
		{
			SW_LOG_WARNING("[MINE] floor=%d 자리가 모자람 %d/%d", CurrentFloorIndex + 1, Done, Row.Count);
		}
	}
}

void USoulFloorDirectorComponent::NotifyRoomEventUsed(ASoulRoomEventObject* Object)
{
	// 그 층 이벤트 오브젝트를 모두 쓰면 1회 보너스 경험치 (EventSetBonusExp, D16)
	if (bEventSetBonusGiven || !FlowData || FlowData->EventSetBonusExp <= 0 || FloorEventObjects.IsEmpty())
		return;
	for (const TWeakObjectPtr<ASoulRoomEventObject>& Event : FloorEventObjects)
	{
		if (Event.IsValid() && !Event->IsSpent())
			return;
	}
	bEventSetBonusGiven = true;
	if (USoulProgressionComponent* Progression = USoulProgressionComponent::Find(PlayerPawn.Get()))
	{
		Progression->AddExp(FlowData->EventSetBonusExp, TEXT("EventSet"));
	}
	SW_LOG("[EVENT] floor=%d 이벤트 전부 완료 보너스 exp=%d", CurrentFloorIndex + 1, FlowData->EventSetBonusExp);
}

void USoulFloorDirectorComponent::ApplyDescendHeal()
{
	if (!bDescendHealPending)
		return;
	bDescendHealPending = false;
	if (!FlowData || FlowData->DescendHealRatio <= 0.f || !FlowData->DescendHealEffect)
		return;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerPawn.Get());
	if (!ASC)
		return;
	const float Before = ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute());
	const float Amount = ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute()) * FlowData->DescendHealRatio;
	ASoulRoomEvent_Effect::ApplyMagnitudeEffect(ASC, FlowData->DescendHealEffect, SoulRunTags::Magnitude_Heal, Amount);
	SW_LOG("[FLOOR] descend heal hp %.0f -> %.0f", Before, ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute()));
}

void USoulFloorDirectorComponent::RunSelfTestEvents(int32 Step)
{
#if !UE_BUILD_SHIPPING
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerPawn.Get());
	auto SetHealth = [ASC](float Value) { if (ASC) ASC->SetNumericAttributeBase(USoulAttributeSet::GetHealthAttribute(), Value); };
	// 2층 준비 뒤에는 RunSelfTestStep이 0부터 다시 부르므로 2층 단계로 넘긴다
	if (Step == 0 && CurrentFloorIndex >= 1)
	{
		Step = 5;
	}
	switch (Step)
	{
	case 0:
		SetHealth(400.f);
		LogPlayerHealth(TEXT("events start"));
		GEngine->Exec(GetWorld(), TEXT("soul.Event.Force Heal"));
		ScheduleSelfTest(1, 2.f);
		break;
	case 1:
		LogPlayerHealth(TEXT("after heal"));
		GEngine->Exec(GetWorld(), TEXT("soul.Event.Force Heal"));
		GEngine->Exec(GetWorld(), TEXT("soul.Event.Force Teleport"));
		ScheduleSelfTest(2, 2.5f);
		break;
	case 2:
		LogPlayerHealth(TEXT("after teleport"));
		SoulFloorDirectorDebug::KillAllMobs(GetWorld());
		GEngine->Exec(GetWorld(), TEXT("soul.Mine.Trigger"));
		ScheduleSelfTest(3, 1.5f);
		break;
	case 3:
		LogPlayerHealth(TEXT("after mine"));
		GEngine->Exec(GetWorld(), TEXT("soul.Chest.Open"));
		ScheduleSelfTest(4, 1.f);
		break;
	case 4:
		SetHealth(500.f);
		LogPlayerHealth(TEXT("before descend"));
		RequestDescend();
		break;
	case 5:
		LogPlayerHealth(TEXT("floor 2 ready"));
		SetHealth(1.f);
		GEngine->Exec(GetWorld(), TEXT("soul.Event.Force Trap"));
		ScheduleSelfTest(6, 2.f);
		break;
	default:
		LogPlayerHealth(TEXT("after trap at hp 1"));
		if (const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this))
		{
			SW_LOG("[SELFTEST] ledger=%d", Run->GetLedgerTotal());
		}
		FinishSelfTest();
		break;
	}
#endif
}

bool USoulFloorDirectorComponent::IsLastFloor() const
{
	return FlowData && CurrentFloorIndex >= FlowData->Floors.Num() - 1;
}

void USoulFloorDirectorComponent::LogFloorState(const TCHAR* Label) const
{
	int32 RoomActors = 0;
	int32 RoomsInCombat = 0;
	for (TActorIterator<ASoulPackedLevelActor> It(GetWorld()); It; ++It)
	{
		if (!IsValid(*It))
			continue;
		++RoomActors;
		RoomsInCombat += It->IsInCombat() ? 1 : 0;
	}

	int32 Enemies = 0;
	for (TActorIterator<ASoulCharacterEnemy> It(GetWorld()); It; ++It)
	{
		Enemies += IsValid(*It) ? 1 : 0;
	}

	int32 Overlays = 0;
	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		if (IsValid(*It) && It->GetWorld() == GetWorld() && It->IsInViewport() && It->GetClass()->GetName().StartsWith(TEXT("WBP_Overlay")))
		{
			++Overlays;
		}
	}

	const APawn* Pawn = PlayerPawn.Get();
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<APawn*>(Pawn));
	const int32 Abilities = ASC ? ASC->GetActivatableAbilities().Num() : -1;
	const int32 ActiveEffects = ASC ? ASC->GetActiveGameplayEffects().GetNumGameplayEffects() : -1;
	const float Health = ASC ? ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute()) : -1.f;
	const float MaxHealth = ASC ? ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute()) : -1.f;

	const AGameModeBase* GameMode = Cast<AGameModeBase>(GetOwner());
	const bool bTeamBound = Builder.IsValid() && Builder->OnGenerationFinished.Contains(GameMode, TeamMapReadyFunction);

	SW_LOG("[FLOOR] %s floor=%d roomActors=%d inCombat=%d enemies=%d exit=%d pawn=%s abilities=%d effects=%d hp=%.0f/%.0f overlays=%d teamBound=%d",
		Label, CurrentFloorIndex + 1, RoomActors, RoomsInCombat, Enemies, ExitRoom.IsValid(),
		Pawn ? *Pawn->GetName() : TEXT("null"), Abilities, ActiveEffects, Health, MaxHealth, Overlays, bTeamBound);
}

// ---------- 디버그 ----------

void USoulFloorDirectorComponent::DebugGoToPortal()
{
	if (bTransitioning || !PlayerPawn.IsValid())
	{
		SW_LOG("[FLOOR] DebugGoToPortal 무시: transitioning=%d pawn=%d", bTransitioning, PlayerPawn.IsValid());
		return;
	}

	ASoulFloorPortal* TargetPortal = Portal.IsValid() ? Portal.Get() : SpawnPortal();
	if (!TargetPortal)
		return;

	SW_LOG("[FLOOR] DebugGoToPortal floor=%d", CurrentFloorIndex + 1);
	DebugFace(TargetPortal, 250.f, PlayerPawn->GetActorLocation() - TargetPortal->GetActorLocation());
	// 다음 틱에 트레이스가 포탈을 잡은 뒤 F (보스 인트로로 입력이 잠겨 있으면 먼저 건너뛴다)
	DebugPressPortal(3);
}

void USoulFloorDirectorComponent::DebugPressPortal(int32 Tries)
{
	GetWorld()->GetTimerManager().SetTimer(SelfTestHealTimer, FTimerDelegate::CreateWeakLambda(this, [this, Tries]()
	{
		if (USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this); Pres && Pres->IsIntroPlaying() && Tries > 0)
		{
			Pres->SkipIntro();
			DebugPressPortal(Tries - 1);
			return;
		}
		if (Portal.IsValid())
		{
			DebugFace(Portal.Get(), 250.f, PlayerPawn.IsValid() ? PlayerPawn->GetActorLocation() - Portal->GetActorLocation() : FVector::ForwardVector);
		}
		if (USoulInteractionComponent* Interaction = USoulInteractionComponent::Find(PlayerPawn.Get()))
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, Interaction]()
			{
				const AActor* Target = Interaction->GetTraceTarget();
				SW_LOG("[FLOOR] DebugGoToPortal trace=%s", Target ? *Target->GetName() : TEXT("none"));
				Interaction->DebugPressInteract();
			}));
		}
	}), 0.8f, false);
}

void USoulFloorDirectorComponent::DebugGoToBossRoom()
{
	ASoulPackedLevelActor* Room = ExitRoom.Get();
	if (bTransitioning || !PlayerPawn.IsValid() || !Room)
	{
		SW_LOG("[FLOOR] DebugGoToBossRoom 무시: transitioning=%d pawn=%d exit=%d", bTransitioning, PlayerPawn.IsValid(), Room != nullptr);
		return;
	}

	const FVector Location = Room->GetActorTransform().TransformPosition(FVector(0.f, -900.f, 150.f));
	SW_LOG("[FLOOR] DebugGoToBossRoom floor=%d", CurrentFloorIndex + 1);
	PlayerPawn->TeleportTo(Location, PlayerPawn->GetActorRotation(), false, true);
}

void USoulFloorDirectorComponent::DebugGoToCombatRoom()
{
	if (bTransitioning || !PlayerPawn.IsValid())
		return;

	// 폰에서 가장 가까운, 아직 안 들어간 전투방 (Exit 제외)
	ASoulPackedLevelActor* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (TActorIterator<ASoulPackedLevelActor> It(GetWorld()); It; ++It)
	{
		if (!IsValid(*It) || !It->HasCombat() || *It == ExitRoom.Get() || It->IsVisited())
			continue;
		const float Distance = FVector::Dist(It->GetActorLocation(), PlayerPawn->GetActorLocation());
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = *It;
		}
	}

	if (!Best)
	{
		SW_LOG_WARNING("[FLOOR] DebugGoToCombatRoom: 전투방 없음");
		return;
	}

	SW_LOG("[FLOOR] DebugGoToCombatRoom room=%d", Best->GetRoomId());
	PlayerPawn->TeleportTo(Best->GetActorLocation() + FVector(0.f, 0.f, 150.f), PlayerPawn->GetActorRotation(), false, true);
}

static TAutoConsoleVariable<FString> CVarSoulFloorSelfTestMode(TEXT("soul.Floor.SelfTestMode"), TEXT(""),
	TEXT("셀프 테스트 모드: 비우면 floors(Next와 포탈로 3층), mob(1층 전투방 몹 E2E), boss(층마다 보스 처치 후 포탈, 3층 클리어)"));

void USoulFloorDirectorComponent::StartSelfTest()
{
#if !UE_BUILD_SHIPPING
	bSelfTest = true;
	SW_LOG("[SELFTEST] 켜짐");
	if (!bTransitioning && CurrentFloorIndex != INDEX_NONE)
	{
		GetWorld()->GetTimerManager().SetTimer(SelfTestTimer, this, &ThisClass::RunSelfTestStep, SoulFloorDirector::SelfTestStepDelaySec, false);
	}
#endif
}

void USoulFloorDirectorComponent::ArmSelfTestWatchdog(const TCHAR* Step)
{
#if !UE_BUILD_SHIPPING
	// 단계가 이 시간 안에 다음으로 안 넘어가면 FAIL로 끝낸다
	const FString StepName(Step);
	GetWorld()->GetTimerManager().SetTimer(SelfTestWatchdog, FTimerDelegate::CreateWeakLambda(this, [this, StepName]()
	{
		SW_LOG_ERROR("[SELFTEST] FAIL step=%s floor=%d", *StepName, CurrentFloorIndex + 1);
		if (GIsEditor)
		{
			UKismetSystemLibrary::QuitGame(this, GetPlayerController(), EQuitPreference::Quit, false);
		}
		else
		{
			GLog->Flush();
			FPlatformMisc::RequestExitWithStatus(true, 3);
		}
	}), SoulFloorDirector::SelfTestStepTimeoutSec, false);
#endif
}

void USoulFloorDirectorComponent::ScheduleSelfTest(int32 Step, float Delay)
{
	ArmSelfTestWatchdog(*FString::Printf(TEXT("%s/%d"), SelfTestMode.IsEmpty() ? TEXT("floors") : *SelfTestMode, Step));
	FTimerDelegate Delegate = FTimerDelegate::CreateWeakLambda(this, [this, Step]()
	{
		if (SelfTestMode == TEXT("mob"))
		{
			RunSelfTestMob(Step);
		}
		else if (SelfTestMode == TEXT("boss"))
		{
			RunSelfTestBoss(Step);
		}
		else if (SelfTestMode == TEXT("death"))
		{
			RunSelfTestDeath(Step);
		}
		else if (SelfTestMode == TEXT("garura"))
		{
			RunSelfTestSpawn(Step);
		}
		else if (SelfTestMode == TEXT("bossfront"))
		{
			RunSelfTestBossFront(Step);
		}
		else if (SelfTestMode == TEXT("stats"))
		{
			RunSelfTestStats(Step);
		}
		else if (SelfTestMode == TEXT("events"))
		{
			RunSelfTestEvents(Step);
		}
		else if (SelfTestMode == TEXT("items"))
		{
			RunSelfTestItems(Step);
		}
		else if (SelfTestMode == TEXT("weapon"))
		{
			RunSelfTestWeapon(Step);
		}
		else if (SelfTestMode == TEXT("summon"))
		{
			RunSelfTestSummon(Step);
		}
		else if (SelfTestMode == TEXT("fpath"))
		{
			RunSelfTestFPath(Step);
		}
		else if (SelfTestMode == TEXT("pickupscore"))
		{
			// 런 픽업을 주우면 점수, 인벤토리에서 버린 것을 다시 주우면 점수 없음 (E 리뷰 1)
			USoulInteractionComponent* Interaction = USoulInteractionComponent::Find(PlayerPawn.Get());
			UInventoryComponent* Inventory = Interaction ? Interaction->GetInventory() : nullptr;
			const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
			auto Ledger = [Run]() { return Run ? Run->GetLedgerTotal() : -1; };
			if (Step == 0)
			{
				const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(this);
				FDataTableRowHandle Row;
				Row.DataTable = Config ? Config->ItemTable : nullptr;
				Row.RowName = TEXT("HealSmall");
				SelfTestPickup = USoulItemLibrary::SpawnPickup(this, Config ? Config->ItemPickupClass : nullptr, Row, PlayerPawn->GetActorLocation() + PlayerPawn->GetActorForwardVector() * 150.f, FVector(400.f, 400.f, 600.f), 0.f);
				SW_LOG("[SELFTEST] pickupscore before ledger=%d tagged=%d", Ledger(), SelfTestPickup.IsValid() && SelfTestPickup->ActorHasTag(USoulItemLibrary::ScorePickupTag));
				ScheduleSelfTest(1, 0.5f);
			}
			else if (Step == 1)
			{
				if (Interaction && SelfTestPickup.IsValid())
				{
					Interaction->CollectTarget(SelfTestPickup.Get());
				}
				SW_LOG("[SELFTEST] pickupscore after run pickup ledger=%d", Ledger());
				// 인벤토리 첫 항목을 버린다 (플러그인 DropItem이 태그 없는 픽업을 스폰)
				const FStructProperty* DataProp = FindFProperty<FStructProperty>(UInventoryComponent::StaticClass(), TEXT("InventoryData"));
				const FArrayProperty* ArrayProp = DataProp ? FindFProperty<FArrayProperty>(DataProp->Struct, TEXT("ItemDataArray")) : nullptr;
				const FStructProperty* EntryProp = ArrayProp ? CastField<FStructProperty>(ArrayProp->Inner) : nullptr;
				const FStructProperty* IdProp = EntryProp ? FindFProperty<FStructProperty>(EntryProp->Struct, TEXT("ID")) : nullptr;
				if (Inventory && IdProp)
				{
					FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(DataProp->ContainerPtrToValuePtr<void>(Inventory)));
					if (Helper.Num() > 0)
					{
						Inventory->DropItem(*IdProp->ContainerPtrToValuePtr<FGuid>(Helper.GetRawPtr(0)), 1);
					}
				}
				ScheduleSelfTest(2, 0.5f);
			}
			else if (Step == 2)
			{
				AActor* Dropped = nullptr;
				for (TActorIterator<AItemActorBase> It(GetWorld()); It; ++It)
				{
					Dropped = *It;
				}
				SW_LOG("[SELFTEST] pickupscore dropped=%s tagged=%d", Dropped ? *Dropped->GetName() : TEXT("none"), Dropped && Dropped->ActorHasTag(USoulItemLibrary::ScorePickupTag));
				if (Interaction && Dropped)
				{
					Interaction->CollectTarget(Dropped);
				}
				ScheduleSelfTest(3, 0.5f);
			}
			else
			{
				SW_LOG("[SELFTEST] pickupscore after re-pickup ledger=%d", Ledger());
				FinishSelfTest();
			}
		}
		else if (SelfTestMode == TEXT("reenter"))
		{
			// F 리뷰 1, 2, 3: 대시 키를 누른 채 보스방 → 인트로 → 스킵 → 질주 풀림 확인 → 같은 방 재진입 2번 → 인트로와 HP 바 1개 → 보스 경직
			APlayerController* PC = GetPlayerController();
			ASoulPlayerController* SoulPC = Cast<ASoulPlayerController>(PC);
			const UCharacterMovementComponent* Move = PC && PC->GetPawn() ? PC->GetPawn()->FindComponentByClass<UCharacterMovementComponent>() : nullptr;
			if (Step == 0)
			{
				// 팀 PC의 InputConfig(비공개)에서 대시 액션을 찾아 그 키를 실제 키 입력으로 누른다 (FlushPressedKeys가 떼기를 만든다)
				FKey DashKey;
				const FObjectProperty* ConfigProp = SoulPC ? FindFProperty<FObjectProperty>(SoulPC->GetClass(), TEXT("InputConfig")) : nullptr;
				const USoulInputConfig* Config = ConfigProp ? Cast<USoulInputConfig>(ConfigProp->GetObjectPropertyValue_InContainer(SoulPC)) : nullptr;
				const UInputAction* DashAction = Config ? Config->FindAbilityInputActionForTag(SoulTags::InputTags::Dash) : nullptr;
				if (const UEnhancedInputLocalPlayerSubsystem* Input = PC ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr; Input && DashAction)
				{
					const TArray<FKey> Keys = Input->QueryKeysMappedToAction(DashAction);
					DashKey = Keys.IsEmpty() ? FKey() : Keys[0];
				}
				if (PC && DashKey.IsValid())
				{
					PC->InputKey(FInputKeyEventArgs::CreateSimulated(DashKey, IE_Pressed, 1.f));
				}
				FTimerHandle Handle;
				GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, DashKey]()
				{
					const APlayerController* LPC = GetPlayerController();
					const UCharacterMovementComponent* LMove = LPC && LPC->GetPawn() ? LPC->GetPawn()->FindComponentByClass<UCharacterMovementComponent>() : nullptr;
					SW_LOG("[SELFTEST] reenter dashKey=%s speedHeld=%.0f", *DashKey.ToString(), LMove ? LMove->MaxWalkSpeed : -1.f);
					DebugGoToBossRoom();
				}), 0.3f, false);
				ScheduleSelfTest(1, 1.8f);
			}
			else if (Step == 1)
			{
				USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this);
				SW_LOG("[SELFTEST] reenter intro playing=%d", Pres && Pres->IsIntroPlaying());
				GEngine->Exec(GetWorld(), TEXT("soul.Pres.SkipIntro"));
				ScheduleSelfTest(2, 1.5f);
			}
			else if (Step == 2)
			{
				SW_LOG("[SELFTEST] reenter after intro input=%d speed=%.0f", PC && PC->InputEnabled(), Move ? Move->MaxWalkSpeed : -1.f);
				HandleExitEntered(ExitRoom.Get());
				HandleExitEntered(ExitRoom.Get());
				ScheduleSelfTest(3, 1.f);
			}
			else
			{
				const USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this);
				TArray<UUserWidget*> Widgets;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, USoulBossIntroWidget::StaticClass(), false);
				SW_LOG("[SELFTEST] reenter after re-entry playing=%d bossWidgets=%d input=%d", Pres && Pres->IsIntroPlaying(), Widgets.Num(), PC && PC->InputEnabled());
				// 보스 경직: 포이즈 경로로 타격을 넣고 HitReact를 시도한다
				for (TActorIterator<ASoulCharacterEnemy> It(GetWorld()); It; ++It)
				{
					const USoulMobComponent* Mob = USoulMobComponent::Find(*It);
					UAbilitySystemComponent* ASC = It->GetAbilitySystemComponent();
					if (!Mob || Mob->GetRole() != ESoulMobRole::Boss || !ASC)
						continue;
					const FGameplayTagContainer HitReact(SoulTags::Combats::HitReact);
					const bool bBlockedAtStart = ASC->AreAbilityTagsBlocked(HitReact);
					int32 StaggerHit = -1;
					for (int32 Hit = 1; Hit <= 60 && StaggerHit < 0; ++Hit)
					{
						USoulCombatFeelSubsystem::NotifyDamage(PC ? PC->GetPawn() : nullptr, *It, 1.f, false, false, FGameplayEffectContextHandle());
						ASC->TryActivateAbilitiesByTag(HitReact);
						if (ASC->HasMatchingGameplayTag(SoulTags::Combats::HitReact))
						{
							StaggerHit = Hit;
						}
					}
					SW_LOG("[SELFTEST] reenter stagger boss=%s blockedAtStart=%d staggerAtHit=%d", *It->GetName(), bBlockedAtStart, StaggerHit);
					break;
				}
				FinishSelfTest();
			}
		}
		else if (SelfTestMode == TEXT("sabreak"))
		{
			// 보스 SABreak 경직(D23): 1, 2층 보스방 → 인트로 스킵 → 포이즈 돌파까지 때림 → 재생 중인 몽타주와 길이 → 1.5초 뒤 아직 재생 중인지 → 1층은 포탈
			ASoulCharacterEnemy* Boss = nullptr;
			for (TActorIterator<ASoulCharacterEnemy> It(GetWorld()); It; ++It)
			{
				const USoulMobComponent* Mob = USoulMobComponent::Find(*It);
				if (Mob && Mob->GetRole() == ESoulMobRole::Boss && !ISoulCombatInterface::Execute_IsDead(*It))
				{
					Boss = *It;
					break;
				}
			}
			const UAnimInstance* Anim = Boss && Boss->GetMesh() ? Boss->GetMesh()->GetAnimInstance() : nullptr;
			const UAnimMontage* Montage = Anim ? Anim->GetCurrentActiveMontage() : nullptr;
			if (Step == 0)
			{
				DebugGoToBossRoom();
				ScheduleSelfTest(1, 2.f);
			}
			else if (Step == 1)
			{
				GEngine->Exec(GetWorld(), TEXT("soul.Pres.SkipIntro"));
				ScheduleSelfTest(2, 1.5f);
			}
			else if (Step == 2)
			{
				UAbilitySystemComponent* ASC = Boss ? Boss->GetAbilitySystemComponent() : nullptr;
				int32 StaggerHit = -1;
				const FGameplayTagContainer HitReact(SoulTags::Combats::HitReact);
				for (int32 Hit = 1; ASC && Hit <= 80 && StaggerHit < 0; ++Hit)
				{
					USoulCombatFeelSubsystem::NotifyDamage(PlayerPawn.Get(), Boss, 1.f, false, false, FGameplayEffectContextHandle());
					ASC->TryActivateAbilitiesByTag(HitReact);
					StaggerHit = ASC->HasMatchingGameplayTag(SoulTags::Combats::HitReact) ? Hit : -1;
				}
				const UAnimMontage* Now = Anim ? Anim->GetCurrentActiveMontage() : nullptr;
				SW_LOG("[SELFTEST] sabreak floor=%d boss=%s staggerAtHit=%d montage=%s len=%.2f segments=%d", CurrentFloorIndex + 1, Boss ? *Boss->GetName() : TEXT("none"), StaggerHit,
					Now ? *Now->GetName() : TEXT("none"), Now ? Now->GetPlayLength() : -1.f, Now && !Now->SlotAnimTracks.IsEmpty() ? Now->SlotAnimTracks[0].AnimTrack.AnimSegments.Num() : -1);
				ScheduleSelfTest(3, 1.5f);
			}
			else
			{
				SW_LOG("[SELFTEST] sabreak after1.5s montage=%s pos=%.2f", Montage ? *Montage->GetName() : TEXT("none"), Anim && Montage ? Anim->Montage_GetPosition(Montage) : -1.f);
				if (CurrentFloorIndex == 0)
				{
					DebugGoToPortal();
				}
				else
				{
					FinishSelfTest();
				}
			}
		}
		else if (SelfTestMode == TEXT("map"))
		{
			// 사용자 피드백 2, 3: 지도 키로 지도가 열리고 현재 층 방이 나오는지 → 닫기 → 1층은 포탈을 F로 들어가 2층에서 반복
			APlayerController* PC = GetPlayerController();
			FKey MapKey;
			if (const UEnhancedInputLocalPlayerSubsystem* Input = PC ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr; Input && GetMapAction())
			{
				const TArray<FKey> Keys = Input->QueryKeysMappedToAction(GetMapAction());
				MapKey = Keys.IsEmpty() ? FKey() : Keys[0];
			}
			auto PressMapKey = [this, PC, MapKey]()
			{
				if (!PC || !MapKey.IsValid())
					return;
				PC->InputKey(FInputKeyEventArgs::CreateSimulated(MapKey, IE_Pressed, 1.f));
				GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [PC, MapKey]()
				{
					PC->InputKey(FInputKeyEventArgs::CreateSimulated(MapKey, IE_Released, 0.f));
				}));
			};
			const ASoulHUD* HUD = PC ? Cast<ASoulHUD>(PC->GetHUD()) : nullptr;
			const USoulMapSubsystem* MapSubsystem = GetWorld()->GetSubsystem<USoulMapSubsystem>();
			if (Step == 0)
			{
				int32 Highlights = 0;
				int32 ActiveHighlights = 0;
				for (TActorIterator<AActor> It(GetWorld()); It; ++It)
				{
					if (const USoulHighlightComponent* Highlight = It->FindComponentByClass<USoulHighlightComponent>())
					{
						++Highlights;
						ActiveHighlights += Highlight->IsHighlightActive() ? 1 : 0;
					}
				}
				SW_LOG("[SELFTEST] map floor=%d key=%s hint=%d highlights=%d/%d", CurrentFloorIndex + 1, *MapKey.ToString(), MapKeyHint && MapKeyHint->IsInViewport(), ActiveHighlights, Highlights);
				PressMapKey();
				ScheduleSelfTest(1, 0.6f);
			}
			else if (Step == 1)
			{
				TArray<UUserWidget*> MapWidgets;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, MapWidgets, USoulMapWidget::StaticClass(), true);
				SW_LOG("[SELFTEST] map open=%d hasData=%d rooms=%d current=%d widgets=%d cursor=%d", HUD && HUD->IsMapOpen(), MapSubsystem && MapSubsystem->HasMapData(),
					MapSubsystem ? MapSubsystem->GetMapResult().Rooms.Num() : -1, MapSubsystem ? MapSubsystem->GetCurrentRoomId() : -1, MapWidgets.Num(), PC && PC->bShowMouseCursor);
				PressMapKey();
				ScheduleSelfTest(2, 0.6f);
			}
			else
			{
				SW_LOG("[SELFTEST] map closed open=%d input=%d", HUD && HUD->IsMapOpen(), PC && PC->InputEnabled());
				if (CurrentFloorIndex == 0)
				{
					DebugGoToPortal();
				}
				else
				{
					FinishSelfTest();
				}
			}
		}
		else if (SelfTestMode == TEXT("bossskip"))
		{
			// 보스방 → 인트로 1.5초 → 스킵 → 입력과 뷰 타깃 복구 확인
			if (Step == 0)
			{
				DebugGoToBossRoom();
				ScheduleSelfTest(1, 1.5f);
			}
			else if (Step == 1)
			{
				const USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this);
				SW_LOG("[SELFTEST] bossskip playing=%d", Pres && Pres->IsIntroPlaying());
				GEngine->Exec(GetWorld(), TEXT("soul.Pres.SkipIntro"));
				ScheduleSelfTest(2, 1.f);
			}
			else
			{
				const APlayerController* PC = GetPlayerController();
				const USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(this);
				SW_LOG("[SELFTEST] bossskip after: playing=%d input=%d viewTargetIsPawn=%d", Pres && Pres->IsIntroPlaying(), PC && PC->InputEnabled(),
					PC && PC->GetViewTarget() == PC->GetPawn());
				FinishSelfTest();
			}
		}
		else if (SelfTestMode == TEXT("result"))
		{
			// 1층 몹 몇을 잡고 바로 클리어 경로 (BeginClear → EndRun → 결과창 → 마을)
			if (Step == 0)
			{
				DebugGoToCombatRoom();
				ScheduleSelfTest(1, 3.f);
			}
			else if (Step == 1)
			{
				SoulFloorDirectorDebug::KillAllMobs(GetWorld());
				ScheduleSelfTest(2, 1.f);
			}
			else
			{
				BeginClear();
				ArmSelfTestWatchdog(TEXT("result"));
			}
		}
		else
		{
			RunSelfTestFloors();
		}
	});
	GetWorld()->GetTimerManager().SetTimer(SelfTestTimer, Delegate, FMath::Max(Delay, 0.01f), false);
}

void USoulFloorDirectorComponent::RunSelfTestStep()
{
#if !UE_BUILD_SHIPPING
	SelfTestMode = CVarSoulFloorSelfTestMode.GetValueOnGameThread();
	if (SelfTestMode.IsEmpty())
	{
		FParse::Value(FCommandLine::Get(), TEXT("SoulFloorSelfTestMode="), SelfTestMode);
	}
	SW_LOG("[SELFTEST] floor=%d mode=%s", CurrentFloorIndex + 1, SelfTestMode.IsEmpty() ? TEXT("floors") : *SelfTestMode);
	ArmSelfTestWatchdog(TEXT("floor ready"));
	ScheduleSelfTest(0, 0.f);
#endif
}

void USoulFloorDirectorComponent::LogPlayerHealth(const TCHAR* Label) const
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<APawn*>(PlayerPawn.Get()));
	int32 Alive = 0;
	for (TActorIterator<ASoulCharacterEnemy> It(GetWorld()); It; ++It)
	{
		Alive += (IsValid(*It) && !ISoulCombatInterface::Execute_IsDead(*It)) ? 1 : 0;
	}
	SW_LOG("[SELFTEST] %s floor=%d playerHp=%.0f/%.0f aliveMobs=%d", Label, CurrentFloorIndex + 1,
		ASC ? ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute()) : -1.f,
		ASC ? ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute()) : -1.f, Alive);
}

void USoulFloorDirectorComponent::FinishSelfTest()
{
#if !UE_BUILD_SHIPPING
	SW_LOG("[SELFTEST] DONE floor=%d", CurrentFloorIndex + 1);
	GetWorld()->GetTimerManager().ClearTimer(RestartTimer);
	GetWorld()->GetTimerManager().ClearTimer(SelfTestWatchdog);
	UKismetSystemLibrary::QuitGame(this, GetPlayerController(), EQuitPreference::Quit, false);
#endif
}

void USoulFloorDirectorComponent::RunSelfTestFloors()
{
#if !UE_BUILD_SHIPPING
	const int32 Floor = CurrentFloorIndex + 1;
	if (Floor == 1)
	{
		SW_LOG("[SELFTEST] soul.Floor.Next");
		RequestDescend();
	}
	else if (!IsLastFloor())
	{
		SW_LOG("[SELFTEST] portal");
		DebugGoToPortal();
	}
	else
	{
		SW_LOG("[SELFTEST] 마지막 층에서 Next (무시되어야 함)");
		RequestDescend();
		FinishSelfTest();
	}
#endif
}

void USoulFloorDirectorComponent::RunSelfTestMob(int32 Step)
{
#if !UE_BUILD_SHIPPING
	switch (Step)
	{
	case 0:
		DebugGoToCombatRoom();
		ScheduleSelfTest(1, 12.f);
		break;
	case 1:
		LogPlayerHealth(TEXT("mob after fight"));
		SoulFloorDirectorDebug::KillAllMobs(GetWorld());
		ScheduleSelfTest(2, 4.f);
		break;
	default:
		LogPlayerHealth(TEXT("mob after kill"));
		FinishSelfTest();
		break;
	}
#endif
}

void USoulFloorDirectorComponent::RunSelfTestBoss(int32 Step)
{
#if !UE_BUILD_SHIPPING
	switch (Step)
	{
	case 0:
		DebugGoToBossRoom();
		ScheduleSelfTest(1, 10.f);
		break;
	case 1:
		LogPlayerHealth(TEXT("boss after fight"));
		SoulFloorDirectorDebug::KillAllMobs(GetWorld());
		// 마지막 층은 클리어 확정을 본 뒤 끝낸다
		ScheduleSelfTest(2, IsLastFloor() ? 2.f : PortalDelaySec + 1.5f);
		break;
	default:
		if (IsLastFloor())
		{
			const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
			SW_LOG("[SELFTEST] clear=%d runEnded=%d", bClearPending, Run ? Run->GetState().bRunEnded : -1);
			FinishSelfTest();
		}
		else
		{
			SW_LOG("[SELFTEST] portal exists=%d", Portal.IsValid());
			DebugGoToPortal();
		}
		break;
	}
#endif
}

void USoulFloorDirectorComponent::LogPawnPose(const TCHAR* Label) const
{
#if !UE_BUILD_SHIPPING
	const ACharacter* Character = Cast<ACharacter>(PlayerPawn.Get());
	if (!Character)
		return;
	const UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	const UAnimMontage* Montage = AnimInstance ? AnimInstance->GetCurrentActiveMontage() : nullptr;
	SW_LOG("[SELFTEST] %s z=%.1f meshZ=%.1f move=%d montage=%s physics=%d", Label, Character->GetActorLocation().Z, Character->GetMesh()->GetComponentLocation().Z,
		static_cast<int32>(Character->GetCharacterMovement()->MovementMode.GetValue()), Montage ? *Montage->GetName() : TEXT("none"), Character->GetMesh()->IsSimulatingPhysics());
#endif
}

void USoulFloorDirectorComponent::RunSelfTestDeath(int32 Step)
{
#if !UE_BUILD_SHIPPING
	switch (Step)
	{
	case 0:
		LogPawnPose(TEXT("death before"));
		GEngine->Exec(GetWorld(), TEXT("soul.Run.ForceEnd Fail"));
		ScheduleSelfTest(1, 0.5f);
		break;
	case 1:
		LogPawnPose(TEXT("death +0.5s"));
		ScheduleSelfTest(2, 2.5f);
		break;
	default:
		// 3.5초에 EndRun(실패)와 재시작 요청이 오면 RestartDungeonMap이 테스트를 끝낸다
		LogPawnPose(TEXT("death +3.0s"));
		break;
	}
#endif
}

void USoulFloorDirectorComponent::RunSelfTestSpawn(int32 Step)
{
#if !UE_BUILD_SHIPPING
	switch (Step)
	{
	case 0:
		GEngine->Exec(GetWorld(), TEXT("soul.Mob.Spawn /Game/Blueprints/Character/Enemy/Mob/JunkGarura/BP_Mob_JunkGarura.BP_Mob_JunkGarura_C 900"));
		// 0.5초마다 BT 노드, 속도, 몽타주, 공격 가능 여부를 남긴다 (발사 간격이 어디서 늘어나는지)
		GetWorld()->GetTimerManager().SetTimer(SelfTestInputTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			for (TActorIterator<ASoulCharacterEnemy> It(GetWorld()); It; ++It)
			{
				const USoulMobComponent* Mob = USoulMobComponent::Find(*It);
				const AAIController* AI = Cast<AAIController>(It->GetController());
				const UBehaviorTreeComponent* BT = AI ? Cast<UBehaviorTreeComponent>(AI->GetBrainComponent()) : nullptr;
				const UBTNode* Node = BT ? BT->GetActiveNode() : nullptr;
				const UAnimInstance* Anim = It->GetMesh() ? It->GetMesh()->GetAnimInstance() : nullptr;
				SW_LOG("[SELFTEST] garura t=%.2f node=%s speed=%.0f montage=%d canAttack=%d", GetWorld()->GetTimeSeconds(), Node ? *Node->GetNodeName() : TEXT("none"),
					It->GetVelocity().Size2D(), Anim && Anim->IsAnyMontagePlaying() ? 1 : 0, Mob && Mob->CanAttackNow() ? 1 : 0);
			}
		}), 0.5f, true);
		ScheduleSelfTest(1, 20.f);
		break;
	case 1:
		GetWorld()->GetTimerManager().ClearTimer(SelfTestInputTimer);
		LogPlayerHealth(TEXT("garura after"));
		SoulFloorDirectorDebug::KillAllMobs(GetWorld());
		ScheduleSelfTest(2, 2.f);
		break;
	default:
		FinishSelfTest();
		break;
	}
#endif
}

void USoulFloorDirectorComponent::LogSkillState(const TCHAR* Label) const
{
#if !UE_BUILD_SHIPPING
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<APawn*>(PlayerPawn.Get()));
	if (!ASC)
		return;
	float Remaining = 0.f;
	float Duration = 0.f;
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(SoulTags::Cooldown::Skill_1_0));
	for (const TPair<float, float>& Pair : ASC->GetActiveEffectsTimeRemainingAndDuration(Query))
	{
		Remaining = FMath::Max(Remaining, Pair.Key);
		Duration = FMath::Max(Duration, Pair.Value);
	}
	SW_LOG("[SELFTEST] %s stamina=%.1f/%.0f cd1_0=%.1f/%.1f", Label, ASC->GetNumericAttribute(USoulAttributeSet::GetStaminaAttribute()),
		ASC->GetNumericAttribute(USoulAttributeSet::GetMaxStaminaAttribute()), Remaining, Duration);
#endif
}

void USoulFloorDirectorComponent::RunSelfTestStats(int32 Step)
{
#if !UE_BUILD_SHIPPING
	USoulProgressionComponent* Progression = USoulProgressionComponent::Find(PlayerPawn.Get());
	USoulAbilitySystemComponent* ASC = Cast<USoulAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerPawn.Get()));
	switch (Step)
	{
	case 0:
		GEngine->Exec(GetWorld(), TEXT("SW.Run.Telemetry 1"));
		if (Progression)
		{
			Progression->DumpToLog();
			GEngine->Exec(GetWorld(), TEXT("soul.Prog.SetLevel 5"));
			GEngine->Exec(GetWorld(), TEXT("soul.Prog.SetLevel 10"));
			GEngine->Exec(GetWorld(), TEXT("soul.Prog.SetLevel 1"));
		}
		LogSkillState(TEXT("skill before"));
		if (ASC)
		{
			ASC->AbilityInputTagPressed(SoulTags::InputTags::KEY_1);
			ASC->AbilityInputTagReleased(SoulTags::InputTags::KEY_1);
		}
		ScheduleSelfTest(1, 0.5f);
		break;
	case 1:
		LogSkillState(TEXT("skill +0.5s"));
		ScheduleSelfTest(2, 2.5f);
		break;
	case 2:
		LogSkillState(TEXT("dash before"));
		if (ASC)
		{
			ASC->AbilityInputTagPressed(SoulTags::InputTags::Dash);
			ASC->AbilityInputTagReleased(SoulTags::InputTags::Dash);
		}
		ScheduleSelfTest(3, 0.2f);
		break;
	case 3:
		LogSkillState(TEXT("dash +0.2s"));
		ScheduleSelfTest(4, 3.f);
		break;
	case 4:
		LogSkillState(TEXT("regen +3s"));
		DebugGoToCombatRoom();
		ScheduleSelfTest(5, 4.f);
		break;
	case 5:
		SoulFloorDirectorDebug::KillAllMobs(GetWorld());
		ScheduleSelfTest(6, 3.f);
		break;
	default:
		if (Progression)
		{
			Progression->DumpToLog();
		}
		if (const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this))
		{
			SW_LOG("[SELFTEST] ledger=%d", Run->GetLedgerTotal());
		}
		FinishSelfTest();
		break;
	}
#endif
}

#if !UE_BUILD_SHIPPING
// FInventoryDataArray는 export되지 않아 반사로 개수만 읽는다
static int32 CountInventoryEntries(const UInventoryComponent* Inventory)
{
	const FStructProperty* DataProp = Inventory ? FindFProperty<FStructProperty>(UInventoryComponent::StaticClass(), TEXT("InventoryData")) : nullptr;
	const FArrayProperty* ArrayProp = DataProp ? FindFProperty<FArrayProperty>(DataProp->Struct, TEXT("ItemDataArray")) : nullptr;
	if (!ArrayProp)
		return -1;
	FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(DataProp->ContainerPtrToValuePtr<void>(Inventory)));
	return Helper.Num();
}

static int32 CountPickups(UWorld* World)
{
	int32 Count = 0;
	for (TActorIterator<AItemActorBase> It(World); It; ++It)
	{
		++Count;
	}
	return Count;
}
#endif

void USoulFloorDirectorComponent::RunSelfTestItems(int32 Step)
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = PlayerPawn.Get();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	USoulInteractionComponent* Interaction = USoulInteractionComponent::Find(Pawn);
	UInventoryComponent* Inventory = Interaction ? Interaction->GetInventory() : nullptr;
	switch (Step)
	{
	case 0:
	{
		// 층 1: 포션을 앞에 떨어뜨리고 F 경로로 줍고 퀵슬롯으로 쓴다
		if (ASC)
		{
			ASC->SetNumericAttributeBase(USoulAttributeSet::GetHealthAttribute(), 300.f);
		}
		const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(this);
		if (Config && Config->ItemTable && Pawn)
		{
			FDataTableRowHandle Row;
			Row.DataTable = Config->ItemTable;
			Row.RowName = TEXT("HealSmall");
			SelfTestPickup = USoulItemLibrary::SpawnPickup(this, Config->ItemPickupClass, Row, Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 150.f, FVector(400.f, 400.f, 600.f), 0.f);
		}
		SW_LOG("[SELFTEST] items spawn pickup=%s inventory=%s", SelfTestPickup.IsValid() ? *SelfTestPickup->GetName() : TEXT("none"), Inventory ? *Inventory->GetName() : TEXT("none"));
		ScheduleSelfTest(1, 0.5f);
		break;
	}
	case 1:
		if (Interaction && SelfTestPickup.IsValid())
		{
			Interaction->CollectTarget(SelfTestPickup.Get());
		}
		ScheduleSelfTest(2, 0.5f);
		break;
	case 2:
		SW_LOG("[SELFTEST] items after collect pickupAlive=%d entries=%d", SelfTestPickup.IsValid() ? 1 : 0, CountInventoryEntries(Inventory));
		LogPlayerHealth(TEXT("items before use"));
		if (Inventory)
		{
			Inventory->UseQuickSlotItem();
		}
		ScheduleSelfTest(3, 0.5f);
		break;
	case 3:
		LogPlayerHealth(TEXT("items after use"));
		SW_LOG("[SELFTEST] items after use entries=%d", CountInventoryEntries(Inventory));
		// 보스 드랍 (DA_Loot_Boss 큰 포션 100%)
		DebugGoToBossRoom();
		ScheduleSelfTest(4, 3.f);
		break;
	case 4:
		SW_LOG("[SELFTEST] items pickups before boss kill=%d", CountPickups(GetWorld()));
		SoulFloorDirectorDebug::KillAllMobs(GetWorld());
		ScheduleSelfTest(5, 2.f);
		break;
	default:
		SW_LOG("[SELFTEST] items pickups after boss kill=%d", CountPickups(GetWorld()));
		FinishSelfTest();
		break;
	}
#endif
}

void USoulFloorDirectorComponent::RunSelfTestWeapon(int32 Step)
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = PlayerPawn.Get();
	USoulInteractionComponent* Interaction = USoulInteractionComponent::Find(Pawn);
	USoulEquipmentComponent* Equipment = USoulEquipmentComponent::Find(Pawn);
	auto LogWeapon = [Pawn, Equipment](const TCHAR* Label)
	{
		FString MeshName = TEXT("none");
		TArray<USkeletalMeshComponent*> Meshes;
		if (Pawn)
		{
			Pawn->GetComponents(Meshes);
		}
		for (const USkeletalMeshComponent* Mesh : Meshes)
		{
			if (Mesh->GetFName() == TEXT("Weapon") && Mesh->GetSkeletalMeshAsset())
			{
				MeshName = Mesh->GetSkeletalMeshAsset()->GetName();
			}
		}
		const USoulEquipmentDefinition* Weapon = Equipment ? Equipment->GetCurrentWeapon() : nullptr;
		SW_LOG("[SELFTEST] weapon %s current=%s mesh=%s", Label, Weapon ? *Weapon->GetName() : TEXT("none"), *MeshName);
	};
	switch (Step)
	{
	case 0:
	{
		LogWeapon(TEXT("start"));
		const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(this);
		if (Config && Config->ItemTable && Pawn)
		{
			FDataTableRowHandle Row;
			Row.DataTable = Config->ItemTable;
			Row.RowName = TEXT("Weapon_012");
			SelfTestPickup = USoulItemLibrary::SpawnPickup(this, Config->WeaponPickupClass, Row, Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 150.f, FVector(400.f, 400.f, 600.f), 0.f);
		}
		ScheduleSelfTest(1, 0.5f);
		break;
	}
	case 1:
		if (Interaction && SelfTestPickup.IsValid())
		{
			Interaction->CollectTarget(SelfTestPickup.Get());
		}
		ScheduleSelfTest(2, 0.5f);
		break;
	case 2:
		LogWeapon(TEXT("after pickup"));
		DebugGoToCombatRoom();
		ScheduleSelfTest(3, 3.f);
		break;
	case 3:
	{
		// 가장 가까운 몹 앞에 서서 평타 6번. 몹 ASC에 걸리는 Spec의 물리와 속성 SetByCaller를 기록한다
		ASoulCharacterEnemy* Target = nullptr;
		float Best = TNumericLimits<float>::Max();
		for (TActorIterator<ASoulCharacterEnemy> It(GetWorld()); It; ++It)
		{
			const float Distance = Pawn ? FVector::Dist(It->GetActorLocation(), Pawn->GetActorLocation()) : 0.f;
			if (Distance < Best)
			{
				Best = Distance;
				Target = *It;
			}
		}
		if (Target && Pawn)
		{
			const FVector Front = Target->GetActorLocation() + Target->GetActorForwardVector() * (Target->GetCapsuleComponent()->GetScaledCapsuleRadius() + 90.f);
			Pawn->TeleportTo(Front, (Target->GetActorLocation() - Front).Rotation(), false, true);
			if (APlayerController* PC = GetPlayerController())
			{
				PC->SetControlRotation((Target->GetActorLocation() - Front).Rotation());
			}
			if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
			{
				TargetASC->OnGameplayEffectAppliedDelegateToSelf.AddWeakLambda(this, [TargetASC](UAbilitySystemComponent* Source, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle)
				{
					FString Parts;
					for (const TTuple<FGameplayTag, FGameplayTag>& Pair : SoulTags::DamageTypes::GetDamageTypes())
					{
						const float Value = Spec.GetSetByCallerMagnitude(Pair.Key, false, 0.f);
						if (Value > 0.f)
						{
							Parts += FString::Printf(TEXT(" %s=%.1f"), *Pair.Key.GetTagName().ToString().RightChop(21), Value);
						}
					}
					if (!Parts.IsEmpty())
					{
						auto R = [TargetASC](const FGameplayAttribute& Attribute) { return TargetASC->GetNumericAttribute(Attribute); };
						SW_LOG("[SELFTEST] weapon hit %s%s hp=%.0f def=%.1f R phys=%.1f fire=%.1f frost=%.1f light=%.1f wind=%.1f holy=%.1f dark=%.1f", *TargetASC->GetAvatarActor()->GetName(), *Parts,
							R(USoulAttributeSet::GetHealthAttribute()), R(USoulAttributeSet::GetDefenseAttribute()), R(USoulAttributeSet::GetPhysicalResistanceAttribute()), R(USoulAttributeSet::GetFireResistanceAttribute()),
							R(USoulAttributeSet::GetFrostResistanceAttribute()), R(USoulAttributeSet::GetLightningResistanceAttribute()), R(USoulAttributeSet::GetWindResistanceAttribute()),
							R(USoulAttributeSet::GetHolyResistanceAttribute()), R(USoulAttributeSet::GetDarkResistanceAttribute()));
					}
				});
			}
			SW_LOG("[SELFTEST] weapon target %s", *Target->GetName());
		}
		GetWorld()->GetTimerManager().SetTimer(SelfTestInputTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (USoulAbilitySystemComponent* ASC = Cast<USoulAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerPawn.Get())))
			{
				ASC->SetNumericAttributeBase(USoulAttributeSet::GetHealthAttribute(), ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute()));
				ASC->AbilityInputTagPressed(SoulTags::InputTags::LMB);
				ASC->AbilityInputTagReleased(SoulTags::InputTags::LMB);
			}
		}), 0.5f, true);
		ScheduleSelfTest(4, 4.f);
		break;
	}
	case 4:
		GetWorld()->GetTimerManager().ClearTimer(SelfTestInputTimer);
		GEngine->Exec(GetWorld(), TEXT("soul.Weapon.Equip"));
		LogWeapon(TEXT("after unequip"));
		GEngine->Exec(GetWorld(), TEXT("soul.Weapon.Equip 029"));
		LogWeapon(TEXT("cheat 029"));
		ScheduleSelfTest(5, 0.5f);
		break;
	default:
		SoulFloorDirectorDebug::KillAllMobs(GetWorld());
		FinishSelfTest();
		break;
	}
#endif
}

void USoulFloorDirectorComponent::DebugFace(const AActor* Target, float Distance, const FVector& FromDirection)
{
	APawn* Pawn = PlayerPawn.Get();
	if (!Pawn || !Target)
		return;
	const FVector Dir = FromDirection.GetSafeNormal2D();
	FVector Stand = Target->GetActorLocation() + Dir * Distance;
	if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation Nav;
		if (NavSys->ProjectPointToNavigation(Stand, Nav, FVector(200.f, 200.f, 500.f)))
		{
			Stand = Nav.Location;
		}
	}
	Stand.Z += Pawn->GetDefaultHalfHeight() + 5.f;
	const FRotator Look = (Target->GetActorLocation() - Stand).GetSafeNormal2D().Rotation();
	Pawn->TeleportTo(Stand, Look, false, true);
	if (APlayerController* PC = GetPlayerController())
	{
		PC->SetControlRotation(FRotator(-15.f, Look.Yaw, 0.f));
	}
}

void USoulFloorDirectorComponent::RunSelfTestFPath(int32 Step)
{
#if !UE_BUILD_SHIPPING
	// 실제 아이템 트레이스와 F 입력 주입으로 상자 열기 → 상자 무기 줍기 → 보스 처치 → 포탈 진입
	USoulInteractionComponent* Interaction = USoulInteractionComponent::Find(PlayerPawn.Get());
	auto TraceName = [Interaction]() { const AActor* T = Interaction ? Interaction->GetTraceTarget() : nullptr; return T ? T->GetName() : FString(TEXT("none")); };
	auto CountPickupsNear = [this](const FVector& At, float Radius)
	{
		int32 Count = 0;
		for (TActorIterator<AItemActorBase> It(GetWorld()); It; ++It)
		{
			Count += FVector::Dist2D(It->GetActorLocation(), At) < Radius ? 1 : 0;
		}
		return Count;
	};
	ASoulTreasureChest* Chest = nullptr;
	for (TActorIterator<ASoulTreasureChest> It(GetWorld()); It; ++It)
	{
		Chest = *It;
	}
	switch (Step)
	{
	case 0:
		if (CurrentFloorIndex > 0)
		{
			SW_LOG("[SELFTEST] fpath portal F ok floor=%d", CurrentFloorIndex + 1);
			FinishSelfTest();
			return;
		}
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerPawn.Get()))
		{
			ASC->AddLooseGameplayTag(SoulTags::States::Invincible);
		}
		// 드랍이 나오는 쪽(+X) 반대에서 상자를 본다 → 열린 상자 너머의 드랍을 트레이스가 잡아야 한다
		DebugFace(Chest, 260.f, Chest ? -Chest->GetActorForwardVector() : FVector::ForwardVector);
		ScheduleSelfTest(1, 0.8f);
		break;
	case 1:
		SW_LOG("[SELFTEST] fpath chest=%s trace=%s", Chest ? *Chest->GetName() : TEXT("none"), *TraceName());
		if (Interaction)
		{
			Interaction->DebugPressInteract();
		}
		ScheduleSelfTest(2, 1.5f);
		break;
	case 2:
		SW_LOG("[SELFTEST] fpath press F: pickupsNearChest=%d trace=%s", Chest ? CountPickupsNear(Chest->GetActorLocation(), 400.f) : -1, *TraceName());
		if (Interaction)
		{
			Interaction->DebugPressInteract();
		}
		ScheduleSelfTest(3, 1.f);
		break;
	case 3:
	{
		const USoulEquipmentComponent* Equipment = USoulEquipmentComponent::Find(PlayerPawn.Get());
		const bool bDefault = !Equipment || !Equipment->GetCurrentWeapon() || (Equipment->GetCatalog() && Equipment->GetCurrentWeapon() == Equipment->GetCatalog()->DefaultWeapon);
		SW_LOG("[SELFTEST] fpath after pickup F: weapon=%s pickupsNearChest=%d trace=%s", Equipment && Equipment->GetCurrentWeapon() ? *Equipment->GetCurrentWeapon()->GetName() : TEXT("none"),
			Chest ? CountPickupsNear(Chest->GetActorLocation(), 400.f) : -1, *TraceName());
		// 무기를 아직 못 주웠으면 남은 드랍 중 가장 가까운 것을 보고 다시 F (최대 3번)
		if (bDefault && Chest && SelfTestRepeat < 3)
		{
			const AActor* Nearest = nullptr;
			for (TActorIterator<AItemActorBase> It(GetWorld()); It; ++It)
			{
				if (FVector::Dist2D(It->GetActorLocation(), Chest->GetActorLocation()) < 400.f && (!Nearest || FVector::Dist(It->GetActorLocation(), PlayerPawn->GetActorLocation()) < FVector::Dist(Nearest->GetActorLocation(), PlayerPawn->GetActorLocation())))
				{
					Nearest = *It;
				}
			}
			if (Nearest)
			{
				++SelfTestRepeat;
				DebugFace(Nearest, 200.f, PlayerPawn->GetActorLocation() - Nearest->GetActorLocation());
				ScheduleSelfTest(2, 0.8f);
				break;
			}
		}
		DebugGoToBossRoom();
		ScheduleSelfTest(4, 3.f);
		break;
	}
	case 4:
		SoulFloorDirectorDebug::KillAllMobs(GetWorld());
		ScheduleSelfTest(5, PortalDelaySec + 1.f);
		break;
	case 5:
		DebugFace(Portal.Get(), 250.f, Portal.IsValid() ? (PlayerPawn.IsValid() ? PlayerPawn->GetActorLocation() - Portal->GetActorLocation() : FVector::ForwardVector) : FVector::ForwardVector);
		ScheduleSelfTest(6, 0.8f);
		break;
	default:
		SW_LOG("[SELFTEST] fpath portal=%s trace=%s", Portal.IsValid() ? *Portal->GetName() : TEXT("none"), *TraceName());
		if (Interaction)
		{
			Interaction->DebugPressInteract();
		}
		// 다음 층이 준비되면 0단계가 성공을 남긴다. 안 넘어가면 워치독이 FAIL
		ArmSelfTestWatchdog(TEXT("fpath/portal"));
		break;
	}
#endif
}

void USoulFloorDirectorComponent::RunSelfTestSummon(int32 Step)
{
#if !UE_BUILD_SHIPPING
	switch (Step)
	{
	case 0:
		// 3층(ThePrimal)까지 바로 내려간다
		if (!IsLastFloor())
		{
			RequestDescend();
			return;
		}
		DebugGoToBossRoom();
		GetWorld()->GetTimerManager().SetTimer(SelfTestHealTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerPawn.Get()))
			{
				ASC->SetNumericAttributeBase(USoulAttributeSet::GetHealthAttribute(), ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute()));
			}
		}), 1.f, true);
		ScheduleSelfTest(1, 4.f);
		break;
	case 1:
		SW_LOG("[SELFTEST] summon before=%d", SoulFloorDirectorDebug::CountSummons(GetWorld()));
		SoulFloorDirectorDebug::DamageBoss(GetWorld(), 0.35f);
		ScheduleSelfTest(2, 2.f);
		break;
	case 2:
		SW_LOG("[SELFTEST] summon after 70%%=%d", SoulFloorDirectorDebug::CountSummons(GetWorld()));
		SoulFloorDirectorDebug::DamageBoss(GetWorld(), 0.05f);
		ScheduleSelfTest(3, 1.f);
		break;
	case 3:
		SW_LOG("[SELFTEST] summon repeat above 40%%=%d", SoulFloorDirectorDebug::CountSummons(GetWorld()));
		SoulFloorDirectorDebug::DamageBoss(GetWorld(), 0.25f);
		ScheduleSelfTest(4, 2.f);
		break;
	case 4:
		SW_LOG("[SELFTEST] summon after 40%%=%d", SoulFloorDirectorDebug::CountSummons(GetWorld()));
		if (ASoulCharacterEnemy* Boss = SoulFloorDirectorDebug::DamageBoss(GetWorld(), 0.f))
		{
			Boss->Die();
		}
		ScheduleSelfTest(5, 3.f);
		break;
	default:
		GetWorld()->GetTimerManager().ClearTimer(SelfTestHealTimer);
		SW_LOG("[SELFTEST] summon after boss death=%d clear=%d", SoulFloorDirectorDebug::CountSummons(GetWorld()), bClearPending);
		FinishSelfTest();
		break;
	}
#endif
}

void USoulFloorDirectorComponent::RunSelfTestBossFront(int32 Step)
{
#if !UE_BUILD_SHIPPING
	switch (Step)
	{
	case 0:
		// 층마다 체력을 채워 세 보스를 모두 본다 (디버그 전용)
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerPawn.Get()))
		{
			ASC->SetNumericAttributeBase(USoulAttributeSet::GetHealthAttribute(), ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute()));
		}
		DebugGoToBossRoom();
		ScheduleSelfTest(1, 3.f);
		break;
	case 1:
	{
		// 보스 바로 앞에 세우고 30초 버틴다 (보스가 실제로 맞히는지)
		const ASoulCharacterEnemy* Boss = nullptr;
		for (TActorIterator<ASoulCharacterEnemy> It(GetWorld()); It; ++It)
		{
			const USoulMobComponent* Mob = USoulMobComponent::Find(*It);
			if (Mob && Mob->GetRole() == ESoulMobRole::Boss)
			{
				Boss = *It;
			}
		}
		if (Boss && PlayerPawn.IsValid())
		{
			const float Gap = Boss->GetCapsuleComponent()->GetScaledCapsuleRadius() + 150.f;
			const FVector Front = Boss->GetActorLocation() + Boss->GetActorForwardVector() * Gap;
			PlayerPawn->TeleportTo(FVector(Front.X, Front.Y, Front.Z + 50.f), (Boss->GetActorLocation() - Front).Rotation(), false, true);
			SW_LOG("[SELFTEST] bossfront %s gap=%.0f", *Boss->GetName(), Gap);
		}
		else
		{
			SW_LOG_ERROR("[SELFTEST] bossfront: 보스 없음");
		}
		LogPlayerHealth(TEXT("bossfront start"));
		// 레벨 1 폰은 2, 3층 보스를 30초 못 버티므로 2초마다 체력을 채운다 (맞는지만 본다)
		GetWorld()->GetTimerManager().SetTimer(SelfTestHealTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerPawn.Get()))
			{
				ASC->SetNumericAttributeBase(USoulAttributeSet::GetHealthAttribute(), ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute()));
			}
		}), 2.f, true);
		ScheduleSelfTest(2, 30.f);
		break;
	}
	case 2:
		GetWorld()->GetTimerManager().ClearTimer(SelfTestHealTimer);
		LogPlayerHealth(TEXT("bossfront after 30s"));
		SoulFloorDirectorDebug::KillAllMobs(GetWorld());
		ScheduleSelfTest(3, IsLastFloor() ? 2.f : PortalDelaySec + 1.5f);
		break;
	default:
		if (IsLastFloor())
		{
			FinishSelfTest();
		}
		else
		{
			DebugGoToPortal();
		}
		break;
	}
#endif
}

#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorld GSoulFloorNextCommand(
	TEXT("soul.Floor.Next"),
	TEXT("던전에서 다음 층으로 내려간다 (포탈과 같은 RequestDescend)"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (USoulFloorDirectorComponent* Director = USoulFloorDirectorComponent::Get(World))
		{
			Director->RequestDescend();
		}
		else
		{
			SW_LOG_WARNING("soul.Floor.Next : Director 없음 (L_Dungeon이 아님)");
		}
	}));

static FAutoConsoleCommandWithWorld GSoulFloorPortalCommand(
	TEXT("soul.Floor.Portal"),
	TEXT("포탈이 없으면 지금 Exit 방에 세우고 폰을 포탈 위로 옮긴다"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (USoulFloorDirectorComponent* Director = USoulFloorDirectorComponent::Get(World))
		{
			Director->DebugGoToPortal();
		}
	}));

static FAutoConsoleCommandWithWorld GSoulFloorSelfTestCommand(
	TEXT("soul.Floor.SelfTest"),
	TEXT("층이 준비될 때마다 Next → 포탈 → 종료 순서로 스스로 진행한다 (자동 실행 확인용)"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		// Director가 있으면 지금 맵에서만 켠다. 없을 때만 다음 L_Dungeon을 위해 플래그를 남긴다
		if (USoulFloorDirectorComponent* Director = USoulFloorDirectorComponent::Get(World))
		{
			Director->StartSelfTest();
		}
		else
		{
			SoulFloorDirector::bSelfTestRequested = true;
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GSoulMobDamageBossCommand(
	TEXT("soul.Mob.DamageBoss"),
	TEXT("soul.Mob.DamageBoss <비율> : 살아 있는 보스 체력을 최대 체력 x 비율만큼 깎는다 (소환 임계 확인)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		SoulFloorDirectorDebug::DamageBoss(World, Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.3f);
	}));

static FAutoConsoleCommandWithWorld GSoulFloorGoBossCommand(
	TEXT("soul.Floor.GoBoss"),
	TEXT("폰을 Exit 방(보스 방)으로 옮긴다"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (USoulFloorDirectorComponent* Director = USoulFloorDirectorComponent::Get(World))
		{
			Director->DebugGoToBossRoom();
		}
	}));

static FAutoConsoleCommandWithWorld GSoulMobKillAllCommand(
	TEXT("soul.Mob.KillAll"),
	TEXT("살아 있는 적을 모두 사망 처리한다 (사망 몽타주, 방 전투 종료 경로를 탄다)"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		SoulFloorDirectorDebug::KillAllMobs(World);
	}));

static FAutoConsoleCommandWithWorldAndArgs GSoulMobSpawnCommand(
	TEXT("soul.Mob.Spawn"),
	TEXT("soul.Mob.Spawn <클래스 경로> [거리] : 폰 앞에 몹을 스폰한다 (런 GameMode에서만)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
		UClass* Class = Args.Num() > 0 ? LoadClass<ASoulCharacterEnemy>(nullptr, *Args[0]) : nullptr;
		if (!Pawn || !Class)
		{
			SW_LOG_WARNING("soul.Mob.Spawn : 폰 또는 클래스 없음 (%s)", Args.Num() > 0 ? *Args[0] : TEXT(""));
			return;
		}
		const float Distance = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 600.f;
		const FVector Location = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * Distance + FVector(0.f, 0.f, 100.f);
		const FRotator Rotation = (Pawn->GetActorLocation() - Location).Rotation();
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AActor* Spawned = World->SpawnActor<ASoulCharacterEnemy>(Class, Location, FRotator(0.f, Rotation.Yaw, 0.f), Params);
		SW_LOG("[MOB] spawn %s", Spawned ? *Spawned->GetName() : TEXT("failed"));
	}));

static FAutoConsoleCommandWithWorldAndArgs GSoulEventForce(
	TEXT("soul.Event.Force"),
	TEXT("soul.Event.Force <Heal|Trap|Teleport> : 이 층의 해당 이벤트 오브젝트를 바로 발동한다"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
		const FString Kind = Args.Num() > 0 ? Args[0] : TEXT("");
		for (TActorIterator<ASoulRoomEventObject> It(World); It; ++It)
		{
			if (It->GetClass()->GetName().Contains(Kind) && It->GetState() == ESoulRoomEventState::Idle)
			{
				It->ForceActivate(Pawn);
				return;
			}
		}
		SW_LOG_WARNING("soul.Event.Force : %s 오브젝트 없음 (또는 이미 씀)", *Kind);
	}));

static FAutoConsoleCommandWithWorld GSoulMineTrigger(
	TEXT("soul.Mine.Trigger"),
	TEXT("폰을 가장 가까운 지뢰 위로 옮기고 밟은 것처럼 발동한다"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
		ASoulFloorMine* Best = nullptr;
		float BestDistance = TNumericLimits<float>::Max();
		for (TActorIterator<ASoulFloorMine> It(World); It; ++It)
		{
			const float Distance = Pawn ? FVector::Dist(It->GetActorLocation(), Pawn->GetActorLocation()) : 0.f;
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = *It;
			}
		}
		if (!Best || !Pawn)
		{
			SW_LOG_WARNING("soul.Mine.Trigger : 지뢰 없음");
			return;
		}
		Pawn->TeleportTo(Best->GetActorLocation() + FVector(0.f, 0.f, Pawn->GetDefaultHalfHeight() + 20.f), Pawn->GetActorRotation(), false, true);
		Best->ForceTrigger();
	}));

static FAutoConsoleCommandWithWorld GSoulChestOpen(
	TEXT("soul.Chest.Open"),
	TEXT("이 층의 보물상자를 연다"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
		for (TActorIterator<ASoulTreasureChest> It(World); It; ++It)
		{
			if (USoulInteractableComponent* Interactable = It->FindComponentByClass<USoulInteractableComponent>())
			{
				Interactable->Interact(Pawn);
				return;
			}
		}
		SW_LOG_WARNING("soul.Chest.Open : 상자 없음");
	}));
#endif
