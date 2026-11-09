#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulProgressionData.generated.h"

class UCurveTable;
class UGameplayEffect;
class USoulLevelBarWidget;

/** 런 레벨과 경험치 (DA_Progression). DA_RunConfig가 가리킨다 */
UCLASS(BlueprintType)
class PROJECTSW_API USoulProgressionData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 레벨별 MaxHealth, AttackPower, Defense와 Exp.ToNext 행 (CT_PlayerLevel) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TObjectPtr<UCurveTable> PlayerLevelTable;

	/** 다음 레벨까지 필요한 경험치 행 이름 (PlayerLevelTable) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	FName ToNextRow;

	/** 레벨 스탯 GE (GE_Level_Stats, Instant Override, Spec 레벨 = 플레이어 레벨). 런 시작 치명 20(상수 Override)도 여기서 건다.
	 *  팀 폰의 기본 GE(GE_DefaultAttributesChii, 치명 50)는 빙의 때 걸리고, 이 GE가 다음 틱에 덮는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TSubclassOf<UGameplayEffect> LevelStatsEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level", meta = (ClampMin = "1"))
	int32 MaxLevel = 1;

	/** 레벨업 회복 = 늘어난 최대 체력 + 새 최대 체력 x 이 비율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level", meta = (ClampMin = "0", ClampMax = "1"))
	float LevelUpHealRatio = 0.f;

	/** 몹 처치 경험치 = KillBase(몹 레벨) x ExpWeight x ExpScale (CT_ExpReward) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exp")
	TObjectPtr<UCurveTable> ExpRewardTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exp")
	FName KillBaseRow;

	/** 전체 경험치 배율 (레벨이 어긋날 때 고치는 레버) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exp", meta = (ClampMin = "0"))
	float ExpScale = 1.f;

	/** 하단 경험치 바 (WBP_LevelBar). 비어 있으면 안 띄운다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<USoulLevelBarWidget> LevelBarClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	int32 LevelBarZOrder = 0;

	/** 레벨과 레벨 값을 읽는다 */
	float EvalPlayerLevel(FName Row, int32 Level) const;
	float EvalKillBase(int32 MobLevel) const;
};
