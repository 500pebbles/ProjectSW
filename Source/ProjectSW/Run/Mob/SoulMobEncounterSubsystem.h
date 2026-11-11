#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/PMGResult.h"
#include "Run/Mob/SoulMobTypes.h"
#include "SoulMobEncounterSubsystem.generated.h"

class ASoulPackedLevelActor;
class ASoulCharacterEnemy;
class USoulMobComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FSoulMobKilledSignature, ASoulCharacterEnemy*, const USoulMobComponent*);

/**
 * 층별 몹 배치. Director가 BeginFloor를 불러야만 동작한다(팀 맵은 그대로).
 * 방의 저작 스폰 포인트를 지우고, 전투방에는 층 세트의 런타임 포인트를, Exit 방에는 보스 포인트 1개를 만든다.
 * 실제 스폰은 기존 방 코드(StartCombat)가 한다. 몹 레벨은 USoulMobComponent가 위치로 물어 정한다.
 */
UCLASS()
class PROJECTSW_API USoulMobEncounterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** bValidateNav: NavMesh 빌드가 끝났을 때 true. 후보를 문에서 걸어갈 수 있는 바닥 점만 남긴다 */
	void BeginFloor(const FSoulMobFloorRules& InRules, const FPMGResult& Result, int32 InFloor, int32 Seed, float CellSize, const FVector& Origin, bool bValidateNav);
	void EndFloor();

	bool IsActive() const { return bActive; }

	/** 위치가 속한 방과 역할로 레벨을 정한다. 엘리트는 점수도 덮는다. 층이 비활성이면 0 */
	int32 ResolveLevel(const FVector& Location, ESoulMobRole Role, int32& InOutScore) const;

	float GetDamageScale() const { return bActive ? Rules.DamageScale : 1.f; }
	float GetAttackIntervalScale() const { return bActive ? Rules.AttackIntervalScale : 1.f; }

	/** 몹이 죽었을 때 (경험치, 점수). USoulMobComponent가 부른다 */
	void NotifyMobKilled(ASoulCharacterEnemy* Enemy, const USoulMobComponent* Mob);
	FSoulMobKilledSignature OnMobKilled;

	/** 층에 깔린 몹 포인트 수 (일반, 보스) */
	int32 GetNormalPointCount() const { return NormalPoints; }
	int32 GetBossPointCount() const { return BossPoints; }

	/**
	 * 강화 몹 자리 요청. 위치가 속한 전투방에 남은 몫이 있고 Role이 Normal이면 몫을 하나 쓰고 true.
	 * 몫은 BeginFloor에서 방마다 정한다(포인트가 무작위 순서로 깔려 먼저 스폰되는 일반 몹이 무작위로 뽑힌다)
	 */
	bool ClaimTough(const FVector& Location, ESoulMobRole Role);

	const FSoulMobToughRules& GetToughRules() const { return Rules.Tough; }

	/** 이 층에 정한 강화 몹 몫 합과, 실제로 강화된 몹 수 */
	int32 GetToughPlanned() const { return ToughPlanned; }
	int32 GetToughApplied() const { return ToughApplied; }

protected:
	virtual void Deinitialize() override;

private:
	void RescueStuckMobs();
	int32 FindRoomAt(const FVector& Location) const;

	/** 이 방 문 하나를 방 안쪽으로 당겨 Nav에 붙인 점 (경로 검사 기준점). 문이 없거나 Nav에 붙지 않으면 false */
	bool FindRoomAnchor(int32 RoomId, const FVector& RoomCenter, FVector& OutAnchor) const;

	/** 후보(월드)를 Nav 바닥 점으로. 같은 방이고 Anchor에서 완전한 경로가 있을 때만 true */
	bool FindSpawnSpot(int32 RoomId, const FVector& Candidate, const FVector& Anchor, FVector& OutFloor);

	/** Center(실패한 후보) 주변을 반경을 늘려 가며 찾는다. 링마다 시작 각도는 Stream으로 정한다 */
	bool FindNearbySpot(int32 RoomId, const FVector& Center, const FVector& Anchor, const TArray<FVector>& Accepted, FRandomStream& Stream, FVector& OutFloor);

	/** 후보 검사 집계 (층 로그용). BeginFloor에서 초기화한다 */
	struct FSpawnCheckStats
	{
		int32 Tested = 0;       // 검사한 후보 수
		int32 NoNav = 0;        // 아래에 Nav 없음
		int32 OtherRoom = 0;    // 투영점이 다른 방/복도
		int32 Unreachable = 0;  // 문에서 못 감 (테이블 위, 감옥 안)
		int32 NearbyFilled = 0; // 근처 검색으로 채운 수
		int32 Short = 0;        // 끝내 못 채운 수
	};
	FSpawnCheckStats CheckStats;

	FSoulMobFloorRules Rules;
	FPMGResult MapResult;
	bool bActive = false;
	int32 Floor = 0;
	float CellSize = 0.f;
	FVector Origin = FVector::ZeroVector;

	/** 전투방 진행률 (0~1). 입구에서 가까운 순 */
	TMap<int32, float> RoomProgress;

	int32 NormalPoints = 0;
	int32 BossPoints = 0;

	/** 방 Id → 남은 강화 몹 몫 */
	TMap<int32, int32> ToughQuota;
	int32 ToughPlanned = 0;
	int32 ToughApplied = 0;

	struct FStuckInfo
	{
		FVector LastLocation = FVector::ZeroVector;
		double LastMovedTime = 0.0;
	};
	TMap<TWeakObjectPtr<AActor>, FStuckInfo> StuckTrack;
	FTimerHandle RescueTimer;
};
