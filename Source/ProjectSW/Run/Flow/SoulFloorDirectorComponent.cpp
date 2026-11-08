#include "Run/Flow/SoulFloorDirectorComponent.h"
#include "Run/Flow/SoulDungeonFlowData.h"
#include "Run/Flow/SoulFloorPortal.h"
#include "Run/Core/SoulRunSubsystem.h"
#include "Actor/SoulProjectileBase.h"
#include "Character/SoulCharacterBase.h"
#include "Character/SoulCharacterEnemy.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Map/SoulPackedLevelActor.h"
#include "Player/SoulPlayerController.h"
#include "UI/HUD/SoulHUD.h"
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
	/** 팀 BP_WorkerGameMode가 빌더 완료 이벤트에 묶는 커스텀 이벤트 이름 */
	static const FName TeamMapReadyFunction(TEXT("OnMapReady"));

	/** soul.Floor.SelfTest가 Director보다 먼저 들어왔을 때 기억해 둔다 */
	static bool bSelfTestRequested = false;

	/** 셀프 테스트에서 층이 준비된 뒤 다음 단계까지 (디버그 전용) */
	static constexpr float SelfTestStepDelaySec = 3.f;
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

	if (SoulFloorDirector::bSelfTestRequested || FParse::Param(FCommandLine::Get(), TEXT("SoulFloorSelfTest")))
	{
		bSelfTest = true;
		SW_LOG("[SELFTEST] 켜짐");
	}

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

	if (!Builder.IsValid() || !FlowData || FlowData->Floors.IsEmpty())
	{
		SW_LOG_ERROR("[FLOOR] 시작 불가: Builder=%d FlowData=%d Floors=%d", Builder.IsValid(), FlowData != nullptr, FlowData ? FlowData->Floors.Num() : 0);
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
	const bool bBoundBefore = Builder->OnGenerationFinished.Contains(GameMode, SoulFloorDirector::TeamMapReadyFunction);
	Builder->OnGenerationFinished.Remove(GameMode, SoulFloorDirector::TeamMapReadyFunction);
	const bool bBoundAfter = Builder->OnGenerationFinished.Contains(GameMode, SoulFloorDirector::TeamMapReadyFunction);
	SW_LOG("[FLOOR] Team OnMapReady binding before=%d after=%d", bBoundBefore, bBoundAfter);

	Builder->OnGenerationFinished.AddUniqueDynamic(this, &ThisClass::HandleGenerationFinished);

	const int32 RunFloor = Run ? Run->GetState().Floor : 1;
	bFirstFloor = true;
	bTransitioning = true;
	BeginFloor(FMath::Clamp(RunFloor - 1, 0, FlowData->Floors.Num() - 1));
}

void USoulFloorDirectorComponent::BeginFloor(int32 FloorIndex)
{
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
	PollNavigation();
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
		SW_LOG("[FLOOR] Pawn spawned: %s", Pawn ? *Pawn->GetName() : TEXT("null"));
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
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const float Waited = GetWorld()->GetTimeSeconds() - NavWaitStartTime;
	const bool bBuilding = NavSys && NavSys->IsNavigationBuildInProgress();

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
	if (NewPortal)
	{
		Portal = NewPortal;
		RegisterFloorActor(NewPortal);
	}

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

	GetWorld()->GetTimerManager().ClearTimer(PortalTimer);
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
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
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

	// 같은 틱에 끄면 뗀 키 처리가 꼬일 수 있어 다음 틱에 끈다
	TWeakObjectPtr<APlayerController> WeakPC = PC;
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [WeakPC]()
	{
		if (APlayerController* LockedPC = WeakPC.Get())
		{
			LockedPC->DisableInput(LockedPC);
			if (APawn* Pawn = LockedPC->GetPawn())
			{
				Pawn->DisableInput(LockedPC);
			}
		}
	}));
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
	const bool bTeamBound = Builder.IsValid() && Builder->OnGenerationFinished.Contains(GameMode, SoulFloorDirector::TeamMapReadyFunction);

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

void USoulFloorDirectorComponent::StartSelfTest()
{
	bSelfTest = true;
	SW_LOG("[SELFTEST] 켜짐");
	if (!bTransitioning && CurrentFloorIndex != INDEX_NONE)
	{
		GetWorld()->GetTimerManager().SetTimer(SelfTestTimer, this, &ThisClass::RunSelfTestStep, SoulFloorDirector::SelfTestStepDelaySec, false);
	}
}

void USoulFloorDirectorComponent::RunSelfTestStep()
{
	const int32 Floor = CurrentFloorIndex + 1;
	SW_LOG("[SELFTEST] step floor=%d", Floor);

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
		SW_LOG("[SELFTEST] DONE floor=%d", Floor);
		UKismetSystemLibrary::QuitGame(this, GetPlayerController(), EQuitPreference::Quit, false);
	}
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
#endif
