#pragma once

#include "CoreMinimal.h"
#include "SoulRunTypes.generated.h"

/**
 * 런 한 번의 상태. USoulRunSubsystem(GameInstance)이 들고 있어 맵 이동을 넘어간다.
 * BeginRun이 초기화하고 EndRun이 끝낸다. 층 사이에는 같은 월드라 따로 옮길 것이 없다.
 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulRunState
{
	GENERATED_BODY()

	/** 지금 층 (1부터). 런이 없으면 0 */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 Floor = 0;

	/** 런 시드. 층 시드 = RunSeed + 층 번호 (+ 재시도 횟수) */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 RunSeed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 Level = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 Exp = 0;

	/** BeginRun 시각 (FPlatformTime::Seconds, 실시간). 클리어 시간 = EndRun - BeginRun */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	double StartRealTime = 0.0;

	/** EndRun 시각 (실시간). 끝난 런의 경과 시간을 고정한다 */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	double EndRealTime = 0.0;

	/** BeginRun 이후 true. EndRun 뒤에도 남고 bRunEnded로 끝났음을 본다 */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	bool bActive = false;

	/** 점수 원장 (항목별 합). 정산 규칙(DA_ScoreRules)은 단계 E */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	TMap<FName, int32> Ledger;

	/** EndRun이 한 번 실행되면 true. 이후 EndRun은 무시한다 */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	bool bRunEnded = false;
};
