#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Run/Core/SoulRunTypes.h"
#include "SoulRunSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoulRunEndedSignature, bool, bSuccess);

/**
 * 로그라이크 런 상태 (층, 시드, 레벨, 경험치, 시작 시각). GameInstance에 있어 OpenLevel을 넘어간다.
 * 런이 없을 때 L_Dungeon을 직접 열면 Director가 BeginRun을 불러 1층부터 시작한다.
 */
UCLASS()
class PROJECTSW_API USoulRunSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static USoulRunSubsystem* Get(const UObject* WorldContextObject);

	/** 층 1, Level 1, Exp 0, 새 RunSeed, 시작 시각 = 지금(실시간) */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void BeginRun();

	/** 런을 한 번만 끝낸다. 이후 호출은 무시한다. 끝나면 Level 1, Exp 0으로 돌린다 */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void EndRun(bool bSuccess);

	/** BeginRun 뒤이고 아직 EndRun 전인지 */
	UFUNCTION(BlueprintPure, Category = "Run")
	bool IsRunActive() const { return State.bActive && !State.bRunEnded; }

	UFUNCTION(BlueprintPure, Category = "Run")
	FSoulRunState GetRunState() const { return State; }

	const FSoulRunState& GetState() const { return State; }

	/** Director가 층을 시작할 때 부른다 */
	void SetFloor(int32 NewFloor);

	/** 레벨과 현재 레벨 안 경험치 (USoulProgressionComponent가 쓴다) */
	void SetLevelExp(int32 NewLevel, int32 NewExp);

	/** 점수 원장에 더한다 (런이 끝났으면 무시) */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void AddScore(FName Category, int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetLedgerTotal() const;

	/** BeginRun부터 지금까지의 실시간 (초). 런이 없으면 0 */
	UFUNCTION(BlueprintPure, Category = "Run")
	double GetElapsedRealSeconds() const;

	UPROPERTY(BlueprintAssignable, Category = "Run")
	FSoulRunEndedSignature OnRunEnded;

	/** 치트 soul.Run.ForceEnd Clear [Score]: 다음 정산 점수를 이 값으로 덮는다 (-1이면 계산) */
	void SetForcedScore(int32 InScore) { ForcedScore = InScore; }
	int32 GetForcedScore() const { return ForcedScore; }

private:
	UPROPERTY()
	FSoulRunState State;

	int32 ForcedScore = -1;
};
