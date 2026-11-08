#include "Run/Flow/SoulFloorDirectorComponent.h"
#include "Run/Flow/SoulDungeonFlowData.h"
#include "Run/Flow/SoulFloorPortal.h"
#include "Run/Core/SoulRunSubsystem.h"
#include "Run/Core/SoulRunGameComponent.h"
#include "Run/Core/SoulRunConfig.h"
#include "Run/Core/SoulRunTags.h"
#include "Run/Mob/SoulMobComponent.h"
#include "Run/Mob/SoulMobEncounterSubsystem.h"
#include "Actor/SoulProjectileBase.h"
#include "Character/SoulCharacterBase.h"
#include "Character/SoulCharacterEnemy.h"
#include "Interface/SoulCombatInterface.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Map/SoulPackedLevelActor.h"
#include "Player/SoulPlayerController.h"
#include "UI/HUD/SoulHUD.h"
#include "Utility/SoulGameplayTags.h"
#include "Utility/SoulLog.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Blueprint/UserWidget.h"
#include "Build/PMGMapBuilder.h"
#include "Camera/PlayerCameraManager.h"
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
		AbortFloorStart(TEXT("map settings"));
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

		// 단계 E에서 타운 복귀로 바꾼다. 그 전에는 런을 끝내고 L_Dungeon을 다시 연다
		SW_LOG_ERROR("[FLOOR] 생성 실패, 재시도 소진: %s", *Result.FailReason);
		if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this))
		{
			Run->EndRun(false);
		}
		RestartDungeonMap();
		return;
	}

	PlacePawn(Result);
	LastResult = Result;

	// 층 몹 배치: 방의 저작 포인트를 지우고 층 세트의 포인트를 만든다 (실제 스폰은 방 입장 때)
	if (USoulMobEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<USoulMobEncounterSubsystem>())
	{
		const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
		const int32 Seed = (Run ? Run->GetState().RunSeed : 0) * 31 + CurrentFloorIndex + 1;
		Encounter->BeginFloor(FlowData->Floors[CurrentFloorIndex].MobRules, Result, CurrentFloorIndex + 1, Seed, Builder->Settings->CellSize, Builder->GetActorLocation());
	}

	ExitRoom = FindRoomActor(Result.ExitRoomId);
	if (ExitRoom.IsValid())
	{
		ExitRoom->OnCombatFinished.AddUObject(this, &ThisClass::HandleExitCombatFinished);
	}
	else
	{
		SW_LOG_WARNING("[FLOOR] Exit 방 액터를 찾지 못함 (id=%d)", Result.ExitRoomId);
	}

	LogFloorState(TEXT("Generated"));

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

	bTransitioning = false;
	bFirstFloor = false;

	LogFloorState(TEXT("Ready"));
	OnFloorReady.Broadcast(CurrentFloorIndex + 1);

	// Exit 방에 전투가 없으면 클리어 신호가 오지 않으므로 바로 포탈을 예약한다
	if (ExitRoom.IsValid() && !ExitRoom->HasCombat() && !IsLastFloor())
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
	bTransitioning = true;
	if (bSelfTest)
	{
		ArmSelfTestWatchdog(TEXT("descend"));
	}
	GetWorld()->GetTimerManager().ClearTimer(PortalTimer);

	OnDescendStarted.Broadcast(CurrentFloorIndex + 1);

	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	LockPlayer();
	CurtainOut(CurtainFadeOutSec);

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
	for (const TWeakObjectPtr<AActor>& Actor : FloorActors)
	{
		if (Actor.IsValid())
		{
			ToDestroy.Add(Actor.Get());
		}
	}
	FloorActors.Reset();
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

void USoulFloorDirectorComponent::HandleExitCombatFinished(ASoulPackedLevelActor* Room)
{
	if (bTransitioning || bClearPending || bPlayerDead || Room != ExitRoom.Get())
		return;

	SW_LOG("[FLOOR] Exit 방 전투 종료 floor=%d", CurrentFloorIndex + 1);

	if (IsLastFloor())
	{
		BeginClear();
	}
	else
	{
		SchedulePortal();
	}
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

	FVector Location = ExitRoom->GetActorLocation();
	if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation NavLocation;
		if (NavSys->ProjectPointToNavigation(Location, NavLocation))
		{
			Location = NavLocation.Location;
		}
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
	SW_LOG("[FLOOR] Portal spawned floor=%d at %s", CurrentFloorIndex + 1, *Location.ToCompactString());
	return NewPortal;
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
		GetWorld()->GetTimerManager().SetTimer(RestartTimer, this, &ThisClass::RestartDungeonMap, FMath::Max(ClearRestartDelaySec, 0.01f), false);
	}));
}

// ---------- 사망 ----------

void USoulFloorDirectorComponent::HandlePawnDied(AActor* DeadActor)
{
	if (bClearPending || bPlayerDead)
		return;

	bPlayerDead = true;
	SW_LOG("[FLOOR] Player died floor=%d", CurrentFloorIndex + 1);

	// 전환 중에 죽으면 다음 층이 생성되지 않게 전환 타이머를 멈춘다
	GetWorld()->GetTimerManager().ClearTimer(PortalTimer);
	GetWorld()->GetTimerManager().ClearTimer(CurtainTimer);
	GetWorld()->GetTimerManager().ClearTimer(UnloadTimer);
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
}

void USoulFloorDirectorComponent::FinishDeath()
{
	if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this))
	{
		Run->EndRun(false);
	}

	// 결과창(단계 E) 전까지 임시로 L_Dungeon을 다시 연다
	RestartDungeonMap();
}

void USoulFloorDirectorComponent::RestartDungeonMap()
{
	SW_LOG("[FLOOR] Restart dungeon map (임시)");
	PrepareOpenLevel();
	const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(this);
	if (Config && !Config->DungeonMap.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, Config->DungeonMap);
	}
	else
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
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
	PlayerPawn->TeleportTo(TargetPortal->GetActorLocation() + FVector(0.f, 0.f, 100.f), PlayerPawn->GetActorRotation(), false, true);
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
			FPlatformMisc::RequestExitWithStatus(false, 3);
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
		SoulFloorDirector::bSelfTestRequested = true;
		if (USoulFloorDirectorComponent* Director = USoulFloorDirectorComponent::Get(World))
		{
			Director->StartSelfTest();
		}
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
#endif
