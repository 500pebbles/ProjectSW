#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Run/Save/SoulHallOfFameTypes.h"
#include "SoulHallOfFameSubsystem.generated.h"

class USoulHallOfFameSave;
class USoulHallOfFameRules;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoulHallOfFameChangedSignature);

/**
 * 세이브와 명예의 전당의 유일한 창구. Initialize에서 동기로 1회 읽는다 (본 → 백업 → 새 프로필).
 * 저장 시점: 런 정산 직후, 이름 확정, 의상 변경, 기록 초기화. 본 슬롯을 쓴 뒤 백업 슬롯에도 쓴다.
 */
UCLASS()
class PROJECTSW_API USoulHallOfFameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static USoulHallOfFameSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 끝난 런을 넣는다. 기록에 들어가면 그 행 ID, 아니면 빈 Guid */
	FGuid SubmitRun(const FSoulRunSummary& Summary);

	/** 이름 확정 (공백 제거, 최대 글자 수로 자름). 마지막 이름으로도 남긴다 */
	UFUNCTION(BlueprintCallable, Category = "HallOfFame")
	void SetEntryName(FGuid EntryId, const FString& Name);

	UFUNCTION(BlueprintCallable, Category = "HallOfFame")
	void SetLastCostume(FName CostumeId);

	/** 기록 행을 모두 지운다 (누적 통계는 남긴다) */
	UFUNCTION(BlueprintCallable, Category = "HallOfFame")
	void ResetRecords();

	UFUNCTION(BlueprintPure, Category = "HallOfFame")
	TArray<FSoulHallOfFameEntry> GetTopEntries(int32 Count) const;

	/** 1부터. 없으면 0 */
	UFUNCTION(BlueprintPure, Category = "HallOfFame")
	int32 GetRankOf(FGuid EntryId) const;

	UFUNCTION(BlueprintPure, Category = "HallOfFame")
	int32 GetEntryCount() const;

	UFUNCTION(BlueprintPure, Category = "HallOfFame")
	FString GetLastPlayerName() const;

	UFUNCTION(BlueprintPure, Category = "HallOfFame")
	FName GetLastCostumeId() const;

	UFUNCTION(BlueprintPure, Category = "HallOfFame")
	int32 GetBoardRows() const;

	UFUNCTION(BlueprintPure, Category = "HallOfFame")
	int32 GetNameMaxLength() const;

	/** 읽기에서 백업 복구나 새 프로필이 일어났으면 한 줄 안내 (타이틀에 표시) */
	UFUNCTION(BlueprintPure, Category = "HallOfFame")
	FString GetLoadNotice() const { return LoadNotice; }

	/** 바로 앞 런이 기록에 들어갔으면 그 행 ID (결과창 이름 입력, 마을 강조) */
	FGuid GetLastSubmittedId() const { return LastSubmittedId; }

	/** 치트 강제 종료: 기록을 내지 않았으니 결과창이 직전 기록을 고치지 않게 비운다 */
	void ClearLastSubmitted() { LastSubmittedId.Invalidate(); bLastSubmittedBest = false; }
	bool WasLastSubmittedNewBest() const { return bLastSubmittedBest; }

	UPROPERTY(BlueprintAssignable, Category = "HallOfFame")
	FSoulHallOfFameChangedSignature OnChanged;

	/** 디버그 */
	void AddTestEntries(int32 Count);
	void DumpToLog() const;

	static const TCHAR* SlotName;
	static const TCHAR* BackupSlotName;

private:
	const USoulHallOfFameRules* GetRules() const;
	void Load();
	void Save();
	void SortAndTrim();
	FString CleanName(const FString& Name) const;

	UPROPERTY()
	TObjectPtr<USoulHallOfFameSave> Profile;

	UPROPERTY()
	mutable TObjectPtr<const USoulHallOfFameRules> CachedRules;

	FString LoadNotice;
	FGuid LastSubmittedId;

	/** 디스크의 본 세이브가 정상인가 (백업 회전 조건) */
	bool bMainValid = false;
	bool bLastSubmittedBest = false;
};
