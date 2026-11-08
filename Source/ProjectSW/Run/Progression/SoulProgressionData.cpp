#include "Run/Progression/SoulProgressionData.h"
#include "Run/Progression/SoulLevelBarWidget.h"

#include "Engine/CurveTable.h"
#include "GameplayEffect.h"

static float EvalRow(const UCurveTable* Table, FName Row, float Key)
{
	if (!Table || Row.IsNone())
		return 0.f;
	const FRealCurve* Curve = Table->FindCurve(Row, TEXT("SoulProgression"), false);
	return Curve ? Curve->Eval(Key) : 0.f;
}

float USoulProgressionData::EvalPlayerLevel(FName Row, int32 Level) const
{
	return EvalRow(PlayerLevelTable, Row, Level);
}

float USoulProgressionData::EvalKillBase(int32 MobLevel) const
{
	return EvalRow(ExpRewardTable, KillBaseRow, MobLevel);
}
