#include "Run/Mob/SoulMobEncounterSubsystem.h"
#include "Run/Mob/SoulMobComponent.h"
#include "Run/Mob/SoulSpawnPatternData.h"
#include "Character/SoulCharacterEnemy.h"
#include "Character/SoulEnemySet.h"
#include "Game/SoulGameModeBase.h"
#include "GAS/Data/SoulEnemyTypeInfo.h"
#include "Map/SoulPackedLevelActor.h"
#include "Map/SoulRoomSpawnPointComponent.h"
#include "Interface/SoulCombatInterface.h"
#include "Utility/SoulLog.h"
#include "Run/Core/SoulRunSubsystem.h"
#include "Build/PMGMapBuilder.h"   // PMG::GetConnectorSpawnTransform

#include "NavigationPath.h"        // UNavigationPath::IsValid / IsPartial
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

namespace SoulEncounter
{
	/** 구제: 이만큼도 안 움직이면 멈춘 것으로 본다 (막힘 감지 기준). 시간과 거리는 USoulMobComponent의 Rescue 칸 */
	static constexpr float StuckMoveTolerance = 50.f;

	/** 문 경계선에서 방 안쪽으로 당기는 거리. 문틀 두께보다 커야 한다 */
	static constexpr float AnchorInset = 200.f;
	/** 기준점 투영 높이: 방 바닥(피벗) 위 이만큼에서 위아래로 찾는다 */
	static constexpr float AnchorLift = 100.f;
	static const FVector AnchorExtent(100.f, 100.f, 500.f);
	/** 후보 투영 범위. XY를 넓히면 옆 소품 위 Nav로 붙어 버린다 */
	static const FVector CandidateExtent(100.f, 100.f, 600.f);
	/** 실패한 후보 주변 탐색: 링 간격, 링 수, 링당 방향 수. 최대 반경 = RingStep x RingCount */
	static constexpr float RingStep = 200.f;
	static constexpr int32 RingCount = 3;
	static constexpr int32 RingDirs = 6;
	static constexpr float MinSpacing = 150.f;

	static bool IsFarFromAll(const FVector& Spot, const TArray<FVector>& Accepted)
	{
		return !Accepted.ContainsByPredicate([&Spot](const FVector& A) { return FVector::Dist2D(A, Spot) < MinSpacing; });
	}
}

void USoulMobEncounterSubsystem::Deinitialize()
{
	EndFloor();
	Super::Deinitialize();
}

void USoulMobEncounterSubsystem::EndFloor()
{
	bActive = false;
	RoomProgress.Reset();
	StuckTrack.Reset();
	NormalPoints = 0;
	BossPoints = 0;
	ToughQuota.Reset();
	ToughPlanned = 0;
	ToughApplied = 0;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RescueTimer);
	}
}

void USoulMobEncounterSubsystem::BeginFloor(const FSoulMobFloorRules& InRules, const FPMGResult& Result, int32 InFloor, int32 Seed, float InCellSize, const FVector& InOrigin, bool bValidateNav)
{
	EndFloor();
	Rules = InRules;
	MapResult = Result;
	Floor = InFloor;
	CellSize = InCellSize;
	Origin = InOrigin;
	bActive = true;
	CheckStats = FSpawnCheckStats();

	if (!bValidateNav)
	{
		SW_LOG_WARNING("[ENC] floor=%d NavMesh 빌드가 끝나지 않아 스폰 후보 검사를 생략함", Floor);
	}

	// 새 EnemyType이 GameMode DA에 없으면 FindChecked로 죽으므로 세트를 먼저 검사한다
	const ASoulGameModeBase* GameMode = Cast<ASoulGameModeBase>(UGameplayStatics::GetGameMode(this));
	const USoulEnemyTypeInfo* TypeInfo = GameMode ? GameMode->EnemyTypeInfo.Get() : nullptr;
	auto CheckSet = [TypeInfo](const USoulEnemySet* Set) -> bool
	{
		if (!Set)
			return false;
		for (const TSubclassOf<ASoulCharacterEnemy>& Class : Set->GetEnemyClasses())
		{
			const ASoulCharacterEnemy* CDO = Class ? Class->GetDefaultObject<ASoulCharacterEnemy>() : nullptr;
			if (!CDO || !TypeInfo || !TypeInfo->EnemyDescription.Contains(CDO->GetEnemyType()))
			{
				SW_LOG_ERROR("[ENC] 세트 %s의 %s: EnemyType이 GameMode DA에 없음. 이 세트는 스폰하지 않음", *Set->GetName(), Class ? *Class->GetName() : TEXT("null"));
				return false;
			}
		}
		return !Set->GetEnemyClasses().IsEmpty();
	};
	USoulEnemySet* NormalSet = CheckSet(Rules.NormalSet) ? Rules.NormalSet.Get() : nullptr;
	USoulEnemySet* BossSet = CheckSet(Rules.BossSet) ? Rules.BossSet.Get() : nullptr;

	// 입구에서의 방 거리 (이웃 그래프 BFS)로 전투방 진행률을 정한다
	TMap<int32, int32> Depth;
	TArray<int32> Queue;
	if (Result.Rooms.IsValidIndex(Result.EntranceRoomId))
	{
		Depth.Add(Result.EntranceRoomId, 0);
		Queue.Add(Result.EntranceRoomId);
	}
	for (int32 Head = 0; Head < Queue.Num(); ++Head)
	{
		const int32 Id = Queue[Head];
		for (const int32 Next : Result.Rooms[Id].Neighbors)
		{
			if (Result.Rooms.IsValidIndex(Next) && Result.Rooms[Next].bConnected && !Depth.Contains(Next))
			{
				Depth.Add(Next, Depth[Id] + 1);
				Queue.Add(Next);
			}
		}
	}

	TArray<int32> CombatRooms;
	for (const FPMGResultRoom& Room : Result.Rooms)
	{
		if (Room.bConnected && Rules.CombatRoomTypes.Contains(Room.RoomType))
		{
			CombatRooms.Add(Room.Id);
		}
	}
	CombatRooms.Sort([&Depth](int32 A, int32 B) { return Depth.FindRef(A) < Depth.FindRef(B); });
	for (int32 i = 0; i < CombatRooms.Num(); ++i)
	{
		RoomProgress.Add(CombatRooms[i], CombatRooms.Num() > 1 ? static_cast<float>(i) / (CombatRooms.Num() - 1) : 0.f);
	}

	FRandomStream Stream(Seed);
	int32 RoomCount = 0;

	for (TActorIterator<ASoulPackedLevelActor> It(GetWorld()); It; ++It)
	{
		ASoulPackedLevelActor* RoomActor = *It;
		const int32 RoomId = RoomActor->GetRoomId();
		if (!Result.Rooms.IsValidIndex(RoomId))
			continue;
		const FPMGResultRoom& Room = Result.Rooms[RoomId];

		// 저작 포인트(팀 더미 몹)를 지운다. 층 규칙만 쓴다
		TArray<USoulRoomSpawnPointComponent*> Existing;
		RoomActor->GetComponents(Existing);
		for (USoulRoomSpawnPointComponent* Point : Existing)
		{
			Point->DestroyComponent();
		}

		const FTransform RoomTransform = RoomActor->GetActorTransform();

		// bOnFloor면 Location은 월드 바닥 점, 아니면 방 로컬. Yaw는 항상 방 로컬
		auto AddPoint = [RoomActor, &RoomTransform](const FVector& Location, bool bOnFloor, float Yaw, USoulEnemySet* Set)
			{
				USoulRoomSpawnPointComponent* Point = NewObject<USoulRoomSpawnPointComponent>(RoomActor);
				Point->SetupAttachment(RoomActor->GetRootComponent());
				const FVector Local = bOnFloor ? RoomTransform.InverseTransformPosition(Location) : Location;
				Point->SetRelativeLocationAndRotation(Local, FRotator(0.f, Yaw, 0.f));
				Point->SetEnemySet(Set);
				Point->SetOnFloor(bOnFloor);
				Point->RegisterComponent();
			};

		// 보스: 검사하지 않는다. DA_DungeonFlow의 BossSpawnLocal을 그대로 쓴다
		if (Room.RoomType == Rules.BossRoomType && BossSet)
		{
			AddPoint(Rules.BossSpawnLocal, false, Rules.BossSpawnYaw, BossSet);
			++BossPoints;
			// 보스 몹 BP의 SpawnClearRadius: 입장 전(커튼 뒤)에 스폰 포인트 발밑 소품을 정리한다
			const FVector Point = RoomTransform.TransformPosition(Rules.BossSpawnLocal);
			for (const TSubclassOf<ASoulCharacterEnemy>& BossClass : BossSet->GetEnemyClasses())
			{
				const USoulMobComponent* Defaults = BossClass ? USoulMobComponent::FindDefaults(BossClass) : nullptr;
				if (!Defaults || Defaults->GetSpawnClearRadius() <= 0.f)
					continue;
				const ACharacter* BossDefaults = BossClass->GetDefaultObject<ACharacter>();
				const float HalfHeight = BossDefaults && BossDefaults->GetCapsuleComponent() ? BossDefaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
				const int32 Cleared = USoulMobComponent::ClearRoomProps(RoomActor, Point, Point.Z - HalfHeight, Defaults->GetSpawnClearRadius(), Defaults->GetSpawnClearMeshes());
				SW_LOG("[ENC] floor=%d boss room clear %s room=%s radius=%.0f cleared=%d", Floor, *BossClass->GetName(), *RoomActor->GetName(), Defaults->GetSpawnClearRadius(), Cleared);
			}
			continue;
		}

		// ---------- 전투방 ----------
		if (!NormalSet || !RoomProgress.Contains(RoomId))
			continue;

		const FSoulMobRoomCount* Count = &Rules.SingleRoom;
		const TArray<FVector>* Candidates = Rules.Patterns ? &Rules.Patterns->SingleRoom : nullptr;
		int32 Tough = Rules.Tough.SingleRoom;
		if (Room.Shape == EPMGRoomShape::DualHorizontal || Room.Shape == EPMGRoomShape::DualVertical)
		{
			Count = &Rules.DualRoom;
			Candidates = Rules.Patterns ? &Rules.Patterns->DualRoom : nullptr;
			Tough = Rules.Tough.DualRoom;
		}
		else if (Room.Shape == EPMGRoomShape::Quad)
		{
			Count = &Rules.QuadRoom;
			Candidates = Rules.Patterns ? &Rules.Patterns->QuadRoom : nullptr;
			Tough = Rules.Tough.QuadRoom;
		}
		if (!Candidates || Candidates->IsEmpty())
			continue;

		// 기준점은 후보가 있는 전투방에서만 구한다 (다른 방에서 쓸데없는 경고가 나지 않게)
		FVector Anchor = FVector::ZeroVector;
		const bool bCheck = bValidateNav && FindRoomAnchor(RoomId, RoomTransform.GetLocation(), Anchor);
		if (bValidateNav && !bCheck)
		{
			SW_LOG_WARNING("[ENC] floor=%d room=%d 문 기준점을 Nav에 붙이지 못함, 이 방은 검사 생략", Floor, RoomId);
		}

		const int32 Wanted = Stream.RandRange(Count->Min, FMath::Max(Count->Min, Count->Max));
		if (Candidates->Num() < Wanted)
		{
			SW_LOG_WARNING("[ENC] floor=%d room=%d 후보 %d개가 몹 수 %d보다 적음 (DA_SpawnPatterns)", Floor, RoomId, Candidates->Num(), Wanted);
		}

		// 후보를 시드로 섞고 앞에서부터 검사한다 (Fisher-Yates, 같은 시드 = 같은 순서)
		TArray<FVector> Pool = *Candidates;
		for (int32 i = Pool.Num() - 1; i > 0; --i)
		{
			Pool.Swap(i, Stream.RandRange(0, i));
		}

		TArray<FVector> Accepted;   // bCheck면 월드 바닥 점, 아니면 방 로컬
		for (const FVector& Local : Pool)
		{
			if (Accepted.Num() >= Wanted)
				break;
			if (!bCheck)
			{
				Accepted.Add(Local);
				continue;
			}

			const FVector Candidate = RoomTransform.TransformPosition(Local);
			FVector SpotFloor;
			if (FindSpawnSpot(RoomId, Candidate, Anchor, SpotFloor) && SoulEncounter::IsFarFromAll(SpotFloor, Accepted))
			{
				Accepted.Add(SpotFloor);
			}
			else if (FindNearbySpot(RoomId, Candidate, Anchor, Accepted, Stream, SpotFloor))
			{
				// 테이블 위/감옥 안 후보 → 그 근처 바닥으로 옮긴다
				Accepted.Add(SpotFloor);
				++CheckStats.NearbyFilled;
			}
			// 근처도 전부 막혔으면 이 후보는 버리고 다음 후보로
		}

		if (Accepted.Num() < Wanted)
		{
			CheckStats.Short += Wanted - Accepted.Num();
			SW_LOG_WARNING("[ENC] floor=%d room=%d 스폰 자리 %d/%d (후보·주변 탐색 후에도 부족)", Floor, RoomId, Accepted.Num(), Wanted);
		}

		// 강화 몹 몫은 실제로 붙인 수 기준
		if (const int32 RoomTough = FMath::Min(Tough, Accepted.Num()); RoomTough > 0)
		{
			ToughQuota.Add(RoomId, RoomTough);
			ToughPlanned += RoomTough;
		}

		const FVector Center = RoomTransform.GetLocation();
		for (const FVector& Spot : Accepted)
		{
			// 방 중앙을 보게 한다. bCheck면 월드 방향에서 방 회전을 빼서 로컬 Yaw로 만든다
			const FVector ToCenter = bCheck ? (Center - Spot) : -Spot;
			const float FacingYaw = FMath::RadiansToDegrees(FMath::Atan2(ToCenter.Y, ToCenter.X));
			const float Yaw = bCheck ? FacingYaw - RoomTransform.Rotator().Yaw : FacingYaw;
			AddPoint(Spot, bCheck, Yaw, NormalSet);
			++NormalPoints;
		}
		++RoomCount;
	}

	SW_LOG("[ENC] floor=%d combatRooms=%d normalPoints=%d toughPlanned=%d bossPoints=%d normalSet=%s bossSet=%s", Floor, RoomCount, NormalPoints, ToughPlanned, BossPoints,
		NormalSet ? *NormalSet->GetName() : TEXT("none"), BossSet ? *BossSet->GetName() : TEXT("none"));
	SW_LOG("[ENC] floor=%d validate=%d tested=%d noNav=%d otherRoom=%d unreachable=%d nearbyFilled=%d short=%d", Floor, bValidateNav,
		CheckStats.Tested, CheckStats.NoNav, CheckStats.OtherRoom, CheckStats.Unreachable, CheckStats.NearbyFilled, CheckStats.Short);
	if (!BossSet || BossPoints == 0)
	{
		SW_LOG_WARNING("[ENC] floor=%d 보스 세트(%s)나 보스 포인트(%d)가 없음", Floor, BossSet ? *BossSet->GetName() : TEXT("none"), BossPoints);
	}

	GetWorld()->GetTimerManager().SetTimer(RescueTimer, FTimerDelegate::CreateUObject(this, &ThisClass::RescueStuckMobs), 1.f, true);
}

void USoulMobEncounterSubsystem::NotifyMobKilled(ASoulCharacterEnemy* Enemy, const USoulMobComponent* Mob)
{
	if (!Mob)
		return;
	if (Mob->IsTough())
	{
		SW_LOG("[ENC] tough killed %s score=%d expWeight=%.1f", Enemy ? *Enemy->GetName() : TEXT("?"), Mob->GetScoreValue(), Mob->GetExpWeight());
	}
	if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this); Run && Mob->GetScoreValue() > 0)
	{
		Run->AddScore(Mob->GetRole() == ESoulMobRole::Boss ? FName(TEXT("Boss")) : FName(TEXT("Kill")), Mob->GetScoreValue());
	}
	if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this); Run && Mob->GetRole() != ESoulMobRole::Summon)
	{
		Run->AddKill();
	}
	OnMobKilled.Broadcast(Enemy, Mob);
}

bool USoulMobEncounterSubsystem::ClaimTough(const FVector& Location, ESoulMobRole Role)
{
	if (!bActive || Role != ESoulMobRole::Normal)
		return false;
	int32* Quota = ToughQuota.Find(FindRoomAt(Location));
	if (!Quota || *Quota <= 0)
		return false;
	--*Quota;
	++ToughApplied;
	return true;
}

int32 USoulMobEncounterSubsystem::FindRoomAt(const FVector& Location) const
{
	if (CellSize <= 0.f)
		return INDEX_NONE;

	const FIntPoint Cell(FMath::FloorToInt((Location.X - Origin.X) / CellSize), FMath::FloorToInt((Location.Y - Origin.Y) / CellSize));
	for (const FPMGResultRoom& Room : MapResult.Rooms)
	{
		if (Room.bConnected && Room.Bounds.Contains(Cell))
		{
			return Room.Id;
		}
	}
	return INDEX_NONE;
}

bool USoulMobEncounterSubsystem::FindRoomAnchor(int32 RoomId, const FVector& RoomCenter, FVector& OutAnchor) const
{
	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
		return false;

	for (const FPMGResultDoor& Door : MapResult.Doors)
	{
		if (Door.RoomA != RoomId && Door.RoomB != RoomId)
			continue;

		// 빌더가 문 액터를 놓는 것과 같은 식으로 경계 중점을 구한다
		FPMGResultConnector Edge;
		Edge.CellA = Door.CellA;
		Edge.CellB = Door.CellB;
		const FVector DoorLocation = PMG::GetConnectorSpawnTransform(Edge, CellSize, Origin).GetLocation();

		// 경계선 위는 옆방 Nav에 붙을 수 있으므로 이 방 셀 중심 쪽(벽에 수직)으로 당긴다
		const FIntPoint MyCell = Door.RoomA == RoomId ? Door.CellA : Door.CellB;
		const FVector MyCellCenter = Origin + FVector((MyCell.X + 0.5f) * CellSize, (MyCell.Y + 0.5f) * CellSize, 0.f);
		FVector Inside = DoorLocation + (MyCellCenter - DoorLocation).GetSafeNormal2D() * SoulEncounter::AnchorInset;
		Inside.Z = RoomCenter.Z + SoulEncounter::AnchorLift;

		FNavLocation Nav;
		if (NavSys->ProjectPointToNavigation(Inside, Nav, SoulEncounter::AnchorExtent) && FindRoomAt(Nav.Location) == RoomId)
		{
			OutAnchor = Nav.Location;
			return true;   // 문은 모두 연결되어 있으니 하나면 충분하다
		}
	}
	return false;
}

bool USoulMobEncounterSubsystem::FindSpawnSpot(int32 RoomId, const FVector& Candidate, const FVector& Anchor, FVector& OutFloor)
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
		return false;

	++CheckStats.Tested;

	// 1. 후보 아래 Nav
	FNavLocation Nav;
	if (!NavSys->ProjectPointToNavigation(Candidate, Nav, SoulEncounter::CandidateExtent))
	{
		++CheckStats.NoNav;
		return false;
	}

	// 2. 같은 방인지 (문 너머 복도, 옆방 방지)
	if (FindRoomAt(Nav.Location) != RoomId)
	{
		++CheckStats.OtherRoom;
		return false;
	}

	// 3. 문에서 끝까지 갈 수 있는지. 부분 경로는 "갈 수 없음"이다 (테이블 위 섬, 감옥 안)
	const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), Anchor, Nav.Location);
	if (!Path || !Path->IsValid() || Path->IsPartial())
	{
		++CheckStats.Unreachable;
		return false;
	}

	OutFloor = Nav.Location;
	return true;
}

bool USoulMobEncounterSubsystem::FindNearbySpot(int32 RoomId, const FVector& Center, const FVector& Anchor, const TArray<FVector>& Accepted, FRandomStream& Stream, FVector& OutFloor)
{
	for (int32 Ring = 1; Ring <= SoulEncounter::RingCount; ++Ring)
	{
		const float Radius = SoulEncounter::RingStep * Ring;
		// 시작 각도만 랜덤, 나머지는 균등 간격: 링 하나에서 모든 방향을 고르게 본다
		const float StartAngle = Stream.FRandRange(0.f, 2.f * PI);
		for (int32 Dir = 0; Dir < SoulEncounter::RingDirs; ++Dir)
		{
			const float Angle = StartAngle + 2.f * PI * Dir / SoulEncounter::RingDirs;
			const FVector Probe = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;

			FVector SpotFloor;
			if (FindSpawnSpot(RoomId, Probe, Anchor, SpotFloor) && SoulEncounter::IsFarFromAll(SpotFloor, Accepted))
			{
				OutFloor = SpotFloor;
				return true;
			}
		}
	}
	return false;
}

int32 USoulMobEncounterSubsystem::ResolveLevel(const FVector& Location, ESoulMobRole Role, int32& InOutScore) const
{
	if (!bActive)
		return 0;

	if (Role == ESoulMobRole::Boss)
		return Rules.BossLevel;

	if (Role == ESoulMobRole::Elite && Rules.EliteLevel > 0)
	{
		if (Rules.EliteScore > 0)
		{
			InOutScore = Rules.EliteScore;
		}
		return Rules.EliteLevel;
	}

	if (Rules.NormalLevelMax <= 0)
		return 0;

	const int32 Min = Rules.NormalLevelMin;
	const int32 Max = FMath::Max(Rules.NormalLevelMin, Rules.NormalLevelMax);
	if (Rules.bRandomLevelInRange)
		return FMath::RandRange(Min, Max);

	const float Progress = RoomProgress.FindRef(FindRoomAt(Location));
	return Min + FMath::RoundToInt(Progress * (Max - Min));
}

void USoulMobEncounterSubsystem::RescueStuckMobs()
{
	UWorld* World = GetWorld();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!World || !Player || !NavSys)
		return;

	const double Now = World->GetTimeSeconds();
	for (TActorIterator<ASoulCharacterEnemy> It(World); It; ++It)
	{
		ASoulCharacterEnemy* Enemy = *It;
		const USoulMobComponent* Mob = USoulMobComponent::Find(Enemy);
		if (!Mob || Mob->IsStationary() || ISoulCombatInterface::Execute_IsDead(Enemy) || Mob->RescueStuckSeconds <= 0.f || Mob->RescueDistance <= 0.f)
			continue;

		FStuckInfo& Info = StuckTrack.FindOrAdd(Enemy);
		if (Info.LastMovedTime == 0.0 || FVector::DistSquared(Info.LastLocation, Enemy->GetActorLocation()) > FMath::Square(SoulEncounter::StuckMoveTolerance))
		{
			Info.LastLocation = Enemy->GetActorLocation();
			Info.LastMovedTime = Now;
			continue;
		}

		// 이동도 공격도 없을 때만 (서서 쏘는 원거리 몹은 건너뛴다)
		if (Now - Info.LastMovedTime < Mob->RescueStuckSeconds || Now - Mob->GetLastAttackTime() < Mob->RescueStuckSeconds
			|| FVector::Dist2D(Enemy->GetActorLocation(), Player->GetActorLocation()) < Mob->RescueDistance)
			continue;

		FNavLocation Point;
		if (NavSys->GetRandomReachablePointInRadius(Player->GetActorLocation(), Mob->RescueDistance, Point))
		{
			Enemy->TeleportTo(Point.Location + FVector(0.f, 0.f, Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), Enemy->GetActorRotation(), false, true);
			Info.LastLocation = Enemy->GetActorLocation();
			Info.LastMovedTime = Now;
			SW_LOG("[ENC] rescue %s", *Enemy->GetName());
		}
	}
}
