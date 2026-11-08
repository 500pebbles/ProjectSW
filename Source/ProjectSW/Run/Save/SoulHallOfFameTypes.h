#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SoulHallOfFameTypes.generated.h"

/** 런 통계 (기록 세부와 결과창) */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulRunStats
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Run")
	int32 Kills = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Run")
	float DamageTaken = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Run")
	int32 Events = 0;

	/** 층별 걸린 시간 (실시간 초) */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Run")
	TArray<float> FloorSeconds;
};

/** 점수 내역 한 줄 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulScoreLine
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Score")
	FName Category;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Score")
	int32 Score = 0;
};

/** 끝난 런 한 번의 정산 결과 (결과창과 기록이 같이 쓴다) */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulRunSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 Score = 0;

	/** S, A, B, C. 실패는 None */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	FName RankId;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	float ClearSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 FinalLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 FloorReached = 0;

	/** 의상과 무기는 DA 이름으로만 남긴다 (경로를 저장하면 에셋 이동 때 깨진다) */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	FName CostumeId;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	FName WeaponId;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	FGameplayTag WeaponElementTag;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	TArray<FSoulScoreLine> Breakdown;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	FSoulRunStats Stats;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	TArray<int32> FloorSeeds;
};

/** 명예의 전당 한 행 (저장 대상) */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulHallOfFameEntry
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HallOfFame")
	FGuid EntryId;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HallOfFame")
	FString PlayerName;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HallOfFame")
	int32 Score = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HallOfFame")
	FName RankId;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HallOfFame")
	bool bSuccess = true;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HallOfFame")
	float ClearSeconds = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HallOfFame")
	int32 FinalLevel = 1;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HallOfFame")
	FName CostumeId;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HallOfFame")
	FName WeaponId;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HallOfFame")
	FGameplayTag WeaponElementTag;

	/** 추가 장비 (최대 2, 지금은 비어 있음) */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HallOfFame")
	TArray<FName> ExtraGearIds;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HallOfFame")
	FSoulRunStats Stats;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HallOfFame")
	TArray<int32> FloorSeeds;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HallOfFame")
	TArray<FSoulScoreLine> ScoreBreakdown;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "HallOfFame")
	FDateTime DateUtc;
};
