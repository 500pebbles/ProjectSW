#include "Run/Save/SoulScoreRules.h"

void USoulScoreRules::Calculate_Implementation(const TMap<FName, int32>& Ledger, FSoulRunSummary& Summary) const
{
	Summary.Breakdown.Reset();
	int32 Total = 0;
	for (const TPair<FName, int32>& Pair : Ledger)
	{
		Summary.Breakdown.Add({ Pair.Key, Pair.Value });
		Total += Pair.Value;
	}

	// 레벨과 시간은 클리어만 (실패는 랭크도 없음)
	if (Summary.bSuccess)
	{
		const int32 LevelScore = Summary.FinalLevel * ScorePerLevel;
		const int32 TimeScore = FMath::Clamp(FMath::RoundToInt((TimeTargetSeconds - Summary.ClearSeconds) * TimeScorePerSecond), 0, TimeScoreMax);
		Summary.Breakdown.Add({ TEXT("Level"), LevelScore });
		Summary.Breakdown.Add({ TEXT("Time"), TimeScore });
		Total += LevelScore + TimeScore;
	}

	Summary.Score = Total;
	Summary.RankId = Summary.bSuccess ? FindRank(Total) : NAME_None;
}

FName USoulScoreRules::FindRank(int32 Score) const
{
	FName Best = NAME_None;
	int32 BestMin = TNumericLimits<int32>::Lowest();
	for (const FSoulRankRow& Row : Ranks)
	{
		if (Score >= Row.MinScore && Row.MinScore > BestMin)
		{
			Best = Row.RankId;
			BestMin = Row.MinScore;
		}
	}
	return Best;
}

FLinearColor USoulScoreRules::GetRankColor(FName RankId) const
{
	for (const FSoulRankRow& Row : Ranks)
	{
		if (Row.RankId == RankId)
			return Row.Color;
	}
	return FLinearColor::White;
}
