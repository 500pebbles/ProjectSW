#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Run/Mob/SoulMobTypes.h"
#include "SoulDungeonFlowData.generated.h"

class UPMGSettings;
class UGameplayEffect;
class ASoulRoomEventObject;
class ASoulFloorMine;
class ASoulTreasureChest;
class USoulSpawnPatternData;

/** 이벤트 오브젝트 종류 하나 (#2). 보장 개수를 먼저 놓고 남는 Event 방은 가중치로 채운다 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulEventRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	TSubclassOf<ASoulRoomEventObject> ObjectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (ClampMin = "0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (ClampMin = "0"))
	int32 GuaranteedCount = 0;

	/** 회복, 함정: 최대 체력 비율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	float Magnitude = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (ClampMin = "0"))
	int32 ExpReward = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (ClampMin = "0"))
	int32 ScoreReward = 0;
};

/** 바닥 지뢰 종류 하나 (D22) */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulMineRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mine")
	TSubclassOf<ASoulFloorMine> MineClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mine", meta = (ClampMin = "0"))
	int32 Count = 0;

	/** 최대 체력 비율 피해 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mine", meta = (ClampMin = "0"))
	float DamageRatio = 0.f;
};

/** 층 하나의 설정. 몹 규칙, 이벤트, 지뢰, 로딩 항목은 다음 단계에서 이 구조체에 붙는다 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulFloorDefinition
{
	GENERATED_BODY()

	/** 이 층의 맵 생성 설정 (DA_PMGSettings_Fn). 비어 있으면 그 층을 시작하지 못한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
	TObjectPtr<UPMGSettings> MapSettings;

	/** 이 층의 몹 세트, 수, 레벨, 보스 위치, 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
	FSoulMobFloorRules MobRules;

	/** 이벤트 오브젝트 (Event 방에 하나씩) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
	TArray<FSoulEventRow> EventRows;

	/** 바닥 지뢰 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
	TArray<FSoulMineRow> MineRows;

	/** Treasure 방 상자와 개봉 점수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
	TSubclassOf<ASoulTreasureChest> TreasureChestClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor", meta = (ClampMin = "0"))
	int32 ChestScore = 0;
};

/**
 * 던전 층 배열과 층 전환 값 (DA_DungeonFlow). BP_DungeonGameMode의 USoulFloorDirectorComponent가 읽는다.
 * 값은 전부 이 에셋에서 고친다. C++ 기본값은 0이다.
 */
UCLASS(BlueprintType)
class PROJECTSW_API USoulDungeonFlowData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 층 순서대로. 1층 = 0번 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floors")
	TArray<FSoulFloorDefinition> Floors;

	/** 맵을 지운(Clear) 뒤 다음 층 Generate까지 기다리는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0", Units = "s"))
	float UnloadHoldSec = 0.f;

	/** 생성이 끝난 뒤 NavMesh 재빌드를 기다리는 최대 시간. 넘으면 그냥 연다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0", Units = "s"))
	float NavWaitTimeoutSec = 0.f;

	/** NavMesh 빌드 여부를 확인하는 간격 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0", Units = "s"))
	float NavPollIntervalSec = 0.f;

	/** 빌더의 프레임당 스폰 예산. 0이면 L_Dungeon 빌더에 놓인 값을 그대로 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0", Units = "ms"))
	float SpawnTimeBudgetMs = 0.f;

	/** 폰 도착 위치 = 입구 방 중심 + ArrivalOffset (+ ArrivalDropHeight 위) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	FVector ArrivalOffset = FVector::ZeroVector;

	/** 도착할 때 떨어지는 높이 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0", Units = "cm"))
	float ArrivalDropHeight = 0.f;

	/** 생성 실패 시 Seed+1로 다시 시도하는 횟수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0"))
	int32 GenerateRetryCount = 0;

	/** 층 이동 회복 (D20): 최대 체력 x 비율, 층 이동마다 1회. 비율 0이거나 GE가 비면 건너뛴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0", ClampMax = "1"))
	float DescendHealRatio = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	TSubclassOf<UGameplayEffect> DescendHealEffect;

	/** 방 타입 이름 (C++에 문자열 비교를 두지 않는다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms")
	FName EventRoomType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms")
	FName TreasureRoomType;

	/** 상자 위치 (Treasure 방 로컬 cm). 방 중앙에는 웨이포인트 화로가 있다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms")
	FVector TreasureChestLocal = FVector::ZeroVector;

	/** 이벤트를 전부 했을 때 보너스 경험치 (D16, 0) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms", meta = (ClampMin = "0"))
	int32 EventSetBonusExp = 0;

	/** 지뢰를 놓는 방 타입 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mine")
	TArray<FName> MineRoomTypes;

	/** 지뢰 위치 후보 (DA_MinePatterns, 방 로컬 cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mine")
	TObjectPtr<USoulSpawnPatternData> MinePatterns;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mine", meta = (ClampMin = "0", Units = "cm"))
	float MineDoorClearance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mine", meta = (ClampMin = "0", Units = "cm"))
	float MineObjectClearance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mine", meta = (ClampMin = "0", Units = "cm"))
	float MineSpacing = 0.f;
};
