#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Run/Save/SoulHallOfFameTypes.h"
#include "SoulScoreRules.generated.h"

struct FSoulRunState;

/** 랭크 문턱 한 행 (높은 점수부터) */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulRankRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rank")
	FName RankId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rank")
	int32 MinScore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rank")
	FLinearColor Color = FLinearColor::White;
};

/**
 * 점수 정산 규칙 (DA_ScoreRules, 4.10). 처치, 보스, 상자, 이벤트는 런 중에 원장으로 들어오고
 * 여기서는 줍기 점수와 클리어 정산(레벨, 시간)과 랭크를 정한다. Calculate는 BlueprintNativeEvent.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTSW_API USoulScoreRules : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 줍기 점수 (DT_RunItems 행 이름 → 점수). 포션 소 200, 대 500 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score")
	TMap<FName, int32> PickupScores;

	/** 비기본 무기 첫 획득 점수 (종류당) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score", meta = (ClampMin = "0"))
	int32 WeaponFirstPickupScore = 0;

	/** 무기 첫 획득 점수를 받는 최대 종류 수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score", meta = (ClampMin = "0"))
	int32 MaxScoredWeapons = 0;

	/** 클리어 정산: 최종 레벨 x 이 값 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score", meta = (ClampMin = "0"))
	int32 ScorePerLevel = 0;

	/** 클리어 정산: (TimeTargetSeconds - 클리어 초) x TimeScorePerSecond, 0~TimeScoreMax */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score", meta = (ClampMin = "0", Units = "s"))
	float TimeTargetSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score", meta = (ClampMin = "0"))
	float TimeScorePerSecond = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score", meta = (ClampMin = "0"))
	int32 TimeScoreMax = 0;

	/** 랭크 문턱. 점수가 MinScore 이상인 첫 행. 실패는 랭크 없음 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score")
	TArray<FSoulRankRow> Ranks;

	/** 원장과 정산으로 점수, 내역, 랭크를 채운다. Summary의 bSuccess, ClearSeconds, FinalLevel은 이미 들어 있다 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Score")
	void Calculate(const TMap<FName, int32>& Ledger, UPARAM(ref) FSoulRunSummary& Summary) const;

	UFUNCTION(BlueprintPure, Category = "Score")
	FName FindRank(int32 Score) const;

	UFUNCTION(BlueprintPure, Category = "Score")
	FLinearColor GetRankColor(FName RankId) const;
};
