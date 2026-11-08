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

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

namespace SoulEncounter
{
	/** 구제: 이 시간 동안 거의 안 움직였고 플레이어와 멀면 옮긴다 (디자인 값이 아니라 막힘 감지 기준) */
	static constexpr double StuckSeconds = 20.0;
	static constexpr float StuckMoveTolerance = 50.f;
	static constexpr float StuckFarDistance = 600.f;
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
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RescueTimer);
	}
}

void USoulMobEncounterSubsystem::BeginFloor(const FSoulMobFloorRules& InRules, const FPMGResult& Result, int32 InFloor, int32 Seed, float InCellSize, const FVector& InOrigin)
{
	EndFloor();
	Rules = InRules;
	MapResult = Result;
	Floor = InFloor;
	CellSize = InCellSize;
	Origin = InOrigin;
	bActive = true;

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

		auto AddPoint = [RoomActor](const FVector& Local, float Yaw, USoulEnemySet* Set)
		{
			USoulRoomSpawnPointComponent* Point = NewObject<USoulRoomSpawnPointComponent>(RoomActor);
			Point->SetupAttachment(RoomActor->GetRootComponent());
			Point->SetRelativeLocationAndRotation(Local, FRotator(0.f, Yaw, 0.f));
			Point->SetEnemySet(Set);
			Point->RegisterComponent();
		};

		if (Room.RoomType == Rules.BossRoomType && BossSet)
		{
			AddPoint(Rules.BossSpawnLocal, Rules.BossSpawnYaw, BossSet);
			++BossPoints;
			continue;
		}

		if (!NormalSet || !RoomProgress.Contains(RoomId))
			continue;

		const FSoulMobRoomCount* Count = &Rules.SingleRoom;
		const TArray<FVector>* Candidates = Rules.Patterns ? &Rules.Patterns->SingleRoom : nullptr;
		if (Room.Shape == EPMGRoomShape::DualHorizontal || Room.Shape == EPMGRoomShape::DualVertical)
		{
			Count = &Rules.DualRoom;
			Candidates = Rules.Patterns ? &Rules.Patterns->DualRoom : nullptr;
		}
		else if (Room.Shape == EPMGRoomShape::Quad)
		{
			Count = &Rules.QuadRoom;
			Candidates = Rules.Patterns ? &Rules.Patterns->QuadRoom : nullptr;
		}
		if (!Candidates || Candidates->IsEmpty())
			continue;

		TArray<FVector> Pool = *Candidates;
		const int32 Num = FMath::Min(Stream.RandRange(Count->Min, FMath::Max(Count->Min, Count->Max)), Pool.Num());
		for (int32 i = 0; i < Num; ++i)
		{
			const int32 Pick = Stream.RandRange(0, Pool.Num() - 1);
			const FVector Local = Pool[Pick];
			Pool.RemoveAtSwap(Pick);
			// 방 중앙을 보게 한다
			AddPoint(Local, FMath::RadiansToDegrees(FMath::Atan2(-Local.Y, -Local.X)), NormalSet);
			++NormalPoints;
		}
		++RoomCount;
	}

	SW_LOG("[ENC] floor=%d combatRooms=%d normalPoints=%d bossPoints=%d normalSet=%s bossSet=%s", Floor, RoomCount, NormalPoints, BossPoints,
		NormalSet ? *NormalSet->GetName() : TEXT("none"), BossSet ? *BossSet->GetName() : TEXT("none"));

	GetWorld()->GetTimerManager().SetTimer(RescueTimer, FTimerDelegate::CreateUObject(this, &ThisClass::RescueStuckMobs), 1.f, true);
}

void USoulMobEncounterSubsystem::NotifyMobKilled(ASoulCharacterEnemy* Enemy, const USoulMobComponent* Mob)
{
	if (!Mob)
		return;
	if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this); Run && Mob->GetScoreValue() > 0)
	{
		Run->AddScore(Mob->GetRole() == ESoulMobRole::Boss ? FName(TEXT("Boss")) : FName(TEXT("Kill")), Mob->GetScoreValue());
	}
	OnMobKilled.Broadcast(Enemy, Mob);
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
		if (!Mob || Mob->IsStationary() || ISoulCombatInterface::Execute_IsDead(Enemy))
			continue;

		FStuckInfo& Info = StuckTrack.FindOrAdd(Enemy);
		if (Info.LastMovedTime == 0.0 || FVector::DistSquared(Info.LastLocation, Enemy->GetActorLocation()) > FMath::Square(SoulEncounter::StuckMoveTolerance))
		{
			Info.LastLocation = Enemy->GetActorLocation();
			Info.LastMovedTime = Now;
			continue;
		}

		if (Now - Info.LastMovedTime < SoulEncounter::StuckSeconds || FVector::Dist2D(Enemy->GetActorLocation(), Player->GetActorLocation()) < SoulEncounter::StuckFarDistance)
			continue;

		FNavLocation Point;
		if (NavSys->GetRandomReachablePointInRadius(Player->GetActorLocation(), SoulEncounter::StuckFarDistance, Point))
		{
			Enemy->TeleportTo(Point.Location + FVector(0.f, 0.f, Enemy->GetDefaultHalfHeight()), Enemy->GetActorRotation(), false, true);
			Info.LastLocation = Enemy->GetActorLocation();
			Info.LastMovedTime = Now;
			SW_LOG("[ENC] rescue %s", *Enemy->GetName());
		}
	}
}
