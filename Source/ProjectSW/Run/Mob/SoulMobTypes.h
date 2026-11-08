#pragma once

#include "CoreMinimal.h"
#include "SoulMobTypes.generated.h"

class UAnimMontage;
class UMaterialInterface;
class ASoulCharacterEnemy;
class USoulEnemySet;
class USoulSpawnPatternData;

/** 몹 역할. 레벨과 점수 규칙이 갈린다 */
UENUM(BlueprintType)
enum class ESoulMobRole : uint8
{
	Normal,
	Tank,
	Elite,
	Boss,
	Summon
};

/** 공격 몽타주 하나의 고르기 규칙. 목록에 없는 몽타주는 가중치 1, 거리 제한 없음으로 친다 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulMobAttackRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	TObjectPtr<UAnimMontage> Montage;

	/** 추첨 가중치 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob", meta = (ClampMin = "0"))
	float Weight = 1.f;

	/** 대상과의 거리가 이보다 가까우면 고르지 않는다 (0이면 제한 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob", meta = (ClampMin = "0", Units = "cm"))
	float MinRange = 0.f;

	/** 대상과의 거리가 이보다 멀면 고르지 않는다 (0이면 제한 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob", meta = (ClampMin = "0", Units = "cm"))
	float MaxRange = 0.f;

	/** 한 번 쓴 뒤 다시 고를 수 있을 때까지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob", meta = (ClampMin = "0", Units = "s"))
	float Cooldown = 0.f;

	/** 이 공격의 피해 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob", meta = (ClampMin = "0"))
	float DamageScale = 1.f;
};

/** 체력 임계에서 부르는 소환물 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulMobSummon
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	TSubclassOf<ASoulCharacterEnemy> Class;

	/** 체력 비율이 이 값 이하가 되면 (0~1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob", meta = (ClampMin = "0", ClampMax = "1"))
	float HealthPercent = 0.f;

	/** 소환자 기준 위치 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	FVector LocalOffset = FVector::ZeroVector;

	/** 소환물 레벨 (0이면 소환물 BP 값) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob", meta = (ClampMin = "0"))
	int32 Level = 0;
};

/** 방 크기별 몹 수 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulMobRoomCount
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob", meta = (ClampMin = "0"))
	int32 Min = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob", meta = (ClampMin = "0"))
	int32 Max = 0;
};

/**
 * 강화 몹(정예). 전투방마다 그 방에 스폰된 Role Normal 몹 몇 마리를 골라 체력, 크기, 보상을 키운다.
 * 역할 Elite(Mist)와는 별개다. 수가 0이면 그 방 크기에서는 고르지 않는다
 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulMobToughRules
{
	GENERATED_BODY()

	/** 방 크기별 강화 몹 수 (그 방 일반 몹 수를 넘지 않는다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tough", meta = (ClampMin = "0"))
	int32 SingleRoom = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tough", meta = (ClampMin = "0"))
	int32 DualRoom = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tough", meta = (ClampMin = "0"))
	int32 QuadRoom = 0;

	/** 최대 체력 배율 (레벨 기본값을 넣은 뒤 곱하고 체력을 꽉 채운다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tough", meta = (ClampMin = "0.1"))
	float HealthScale = 3.f;

	/** 크기 배율 (캡슐과 메시가 같이 커진다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tough", meta = (ClampMin = "0.1"))
	float MeshScale = 1.2f;

	/** 처치 경험치 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tough", meta = (ClampMin = "0"))
	float ExpScale = 2.f;

	/** 처치 점수 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tough", meta = (ClampMin = "0"))
	float ScoreScale = 2.f;

	/** 구분용 오버레이 재질 (M_RunHighlight처럼 Color, Intensity, FresnelExponent, BaseGlow 파라미터). 비우면 크기만 바뀐다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tough|Look")
	TObjectPtr<UMaterialInterface> OverlayMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tough|Look")
	FLinearColor OverlayColor = FLinearColor(1.f, 0.12f, 0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tough|Look", meta = (ClampMin = "0"))
	float OverlayIntensity = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tough|Look", meta = (ClampMin = "0.1"))
	float OverlayFresnelExponent = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tough|Look", meta = (ClampMin = "0"))
	float OverlayBaseGlow = 0.05f;
};

/** 층 하나의 몹 규칙 (DA_DungeonFlow.Floors[n].MobRules). USoulMobEncounterSubsystem이 읽는다 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulMobFloorRules
{
	GENERATED_BODY()

	/** 일반 방 몹 세트 (배열 중복이 비중) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	TObjectPtr<USoulEnemySet> NormalSet;

	/** Exit 방 보스 세트 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	TObjectPtr<USoulEnemySet> BossSet;

	/** 몹을 넣는 방 타입 (PMG 방 타입 이름) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	TArray<FName> CombatRoomTypes;

	/** Exit 방 타입 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	FName BossRoomType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Count")
	FSoulMobRoomCount SingleRoom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Count")
	FSoulMobRoomCount DualRoom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Count")
	FSoulMobRoomCount QuadRoom;

	/** 강화 몹(정예): 방마다 일반 몹 중 몇 마리를 체력, 크기, 보상 배율로 키운다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Tough")
	FSoulMobToughRules Tough;

	/** 방 안 스폰 위치 후보 (DA_SpawnPatterns) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Count")
	TObjectPtr<USoulSpawnPatternData> Patterns;

	/** 일반 몹 레벨 범위. 입구에서 먼 전투방일수록 높다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Level", meta = (ClampMin = "0"))
	int32 NormalLevelMin = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Level", meta = (ClampMin = "0"))
	int32 NormalLevelMax = 0;

	/** 켜면 진행률 대신 범위 안 무작위 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Level")
	bool bRandomLevelInRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Level", meta = (ClampMin = "0"))
	int32 BossLevel = 0;

	/** Role이 Elite인 몹의 레벨과 점수 (0이면 덮지 않음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Level", meta = (ClampMin = "0"))
	int32 EliteLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Level", meta = (ClampMin = "0"))
	int32 EliteScore = 0;

	/** 보스 스폰 위치 (방 로컬 cm)와 방향 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Boss")
	FVector BossSpawnLocal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Boss")
	float BossSpawnYaw = 0.f;

	/** 층 전체 피해 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Scale", meta = (ClampMin = "0"))
	float DamageScale = 1.f;

	/** 층 전체 공격 간격 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Scale", meta = (ClampMin = "0"))
	float AttackIntervalScale = 1.f;
};
