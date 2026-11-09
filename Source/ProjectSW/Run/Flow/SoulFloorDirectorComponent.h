#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/PMGResult.h"
#include "SoulFloorDirectorComponent.generated.h"

class APMGMapBuilder;
class APawn;
class APlayerController;
class ASoulFloorPortal;
class UInputAction;
class USoulKeyHintWidget;
class ASoulRoomEventObject;
class ASoulPackedLevelActor;
class UAnimMontage;
class USoulDungeonFlowData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoulFloorEventSignature, int32, Floor);

/**
 * 던전 층 진행 (BP_DungeonGameMode의 컴포넌트). 같은 월드에서 맵을 지우고(Clear) 다음 층 설정으로 다시 만든다(Generate).
 *
 * BeginPlay      → PC 입력 잠금, 커튼 → 다음 틱 StartFirstFloor
 * StartFirstFloor → 런이 없으면 BeginRun → 팀 GM의 OnMapReady 바인딩 해제 → 자기 핸들러 → 층 시작
 * 생성 완료       → 폰 스폰(첫 층) 또는 이동 → NavMesh 대기 → 커튼 해제 → OnFloorReady
 * Exit 방 전투 끝 → 마지막 층이면 클리어, 아니면 포탈 예약
 * 포탈, soul.Floor.Next → RequestDescend → 정리 → 다음 층
 * 폰 OnDied      → 사망 최소 처리 → EndRun(실패) → (결과창 전까지 임시) L_Dungeon 재시작
 */
UCLASS(ClassGroup = (Soul), meta = (BlueprintSpawnableComponent))
class PROJECTSW_API USoulFloorDirectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USoulFloorDirectorComponent();

	/** 지금 월드의 GameMode에 붙은 Director (L_Dungeon이 아니면 null) */
	static USoulFloorDirectorComponent* Get(const UObject* WorldContextObject);

	/** 다음 층으로 내려간다. 전환 중, 클리어 확정 뒤, 사망 뒤, 마지막 층에서는 무시한다 */
	UFUNCTION(BlueprintCallable, Category = "Floor")
	void RequestDescend();

	/** 디버그: 지금 층에서 클리어 경로를 탄다 (soul.Run.ForceEnd Clear) */
	void DebugForceClear();

	/** 층을 바꿀 때 함께 지울 액터 (포탈, 이후 상자와 이벤트 오브젝트) */
	UFUNCTION(BlueprintCallable, Category = "Floor")
	void RegisterFloorActor(AActor* Actor);

	/** 지금 층 (1부터). 시작 전이면 0 */
	UFUNCTION(BlueprintPure, Category = "Floor")
	int32 GetCurrentFloor() const { return CurrentFloorIndex + 1; }

	UFUNCTION(BlueprintPure, Category = "Floor")
	bool IsTransitioning() const { return bTransitioning; }

	/** 층이 준비되어 커튼이 걷히고 입력이 돌아왔을 때 */
	UPROPERTY(BlueprintAssignable, Category = "Floor")
	FSoulFloorEventSignature OnFloorReady;

	/** 층 이동을 시작했을 때 (Floor = 떠나는 층) */
	UPROPERTY(BlueprintAssignable, Category = "Floor")
	FSoulFloorEventSignature OnDescendStarted;

	/** 지금 층 맵 결과와 좌표 (순간이동, 지뢰) */
	const FPMGResult& GetLastResult() const { return LastResult; }
	float GetCellSize() const;
	FVector GetBuilderOrigin() const;
	int32 GetRoomIdAt(const FVector& Location) const;
	ASoulPackedLevelActor* FindRoomActor(int32 RoomId) const;

	/** 디버그: 포탈이 없으면 지금 Exit 방에 세우고, 폰을 포탈 위로 옮긴다 (soul.Floor.Portal) */
	void DebugGoToPortal();

	/** 디버그: 층이 준비될 때마다 스스로 진행한다 (soul.Floor.SelfTest, 모드는 soul.Floor.SelfTestMode) */
	void StartSelfTest();

	/** 디버그: 폰을 Exit 방(보스 방) 안으로 옮긴다 (soul.Floor.GoBoss) */
	void DebugGoToBossRoom();

	/** 디버그: 폰을 가장 가까운 몹 전투방 안으로 옮긴다 */
	void DebugGoToCombatRoom();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 층 배열과 전환 값 (DA_DungeonFlow) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Floor")
	TObjectPtr<USoulDungeonFlowData> FlowData;

	/** 팀 BP_WorkerGameMode가 빌더 완료 이벤트에 묶는 함수 이름. 재생성마다 폰을 다시 만들므로 뗀다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Floor")
	FName TeamMapReadyFunction = TEXT("OnMapReady");

	/** Exit 방 전투가 끝나면 세우는 포탈 (BP_FloorPortal) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Floor")
	TSubclassOf<ASoulFloorPortal> PortalClass;

	// ---------- 임시 값 (단계 F에서 DA_Presentation으로 옮긴다) ----------

	/** 임시 커튼(카메라 페이드)이 검게 되는 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Temporary", meta = (ClampMin = "0", Units = "s"))
	float CurtainFadeOutSec = 0.f;

	/** 커튼이 검게 된 뒤 최소로 유지하는 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Temporary", meta = (ClampMin = "0", Units = "s"))
	float CurtainMinHoldSec = 0.f;

	/** 커튼이 걷히는 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Temporary", meta = (ClampMin = "0", Units = "s"))
	float CurtainFadeInSec = 0.f;

	/** Exit 방 전투가 끝나고 포탈이 서기까지 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Temporary", meta = (ClampMin = "0", Units = "s"))
	float PortalDelaySec = 0.f;

	/** 플레이어 사망 몽타주 (WorkerDeath_Montage). 비어 있으면 재생하지 않는다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Temporary")
	TObjectPtr<UAnimMontage> DeathMontage;

	/** 사망부터 EndRun(실패)까지 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Temporary", meta = (ClampMin = "0", Units = "s"))
	float DeathEndRunDelaySec = 0.f;

	/** 클리어 EndRun 뒤 L_Dungeon을 다시 열기까지 (결과창이 생기면 지운다) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Temporary", meta = (ClampMin = "0", Units = "s"))
	float ClearRestartDelaySec = 0.f;

private:
	void StartFirstFloor();
	void BeginFloor(int32 FloorIndex);
	void GenerateCurrentFloor();

	UFUNCTION()
	void HandleGenerationFinished(const FPMGResult& Result, const TArray<FString>& SpawnErrors);

	void PlacePawn(const FPMGResult& Result);
	void PollNavigation();
	void TryReveal();
	void Reveal();

	void UnloadFloor();
	void DestroyFloorActors();

	void HandleExitCombatFinished(ASoulPackedLevelActor* Room);
	void HandleRoomEnteredTelemetry(ASoulPackedLevelActor* Room);
	void HandleRoomCombatFinishedTelemetry(ASoulPackedLevelActor* Room);
	void LogTelemetry(const FString& Event) const;
	void SchedulePortal();
	ASoulFloorPortal* SpawnPortal();

	/** 포탈 배너, 포탈 위 표시 (DA_Presentation 문구, 키 이름은 입력 매핑) */
	void ShowPortalGuide(ASoulFloorPortal* InPortal) const;

	/** 지도 키 안내 (팀 PC의 MapAction 매핑 키) */
	void ShowMapKeyHint();
	void HideMapKeyHint();
	const UInputAction* GetMapAction() const;
	void BeginClear();

	void HandlePawnDied(AActor* DeadActor);
	void FinishDeath();
	/** 런이 끝난 뒤: 결과창 → 마을 (결과창 클래스가 없으면 바로 마을) */
	void ShowRunResult();
	FName GetLoadingId(int32 FloorIndex) const;
	void HandleExitEntered(ASoulPackedLevelActor* Room);
	void HandlePlayerHealthChanged(const struct FOnAttributeChangeData& Data);
	int32 LastGeneratedSeed = 0;
	bool bDamageTracked = false;

	void LockPlayer();
	void DisableInputAfterFrame(TWeakObjectPtr<APlayerController> WeakPC, uint64 LockFrame);
	void AbortFloorStart(const TCHAR* Reason);

	/** 이번 층 보스 인트로를 이미 돌렸다 */
	bool bBossIntroDone = false;
	void ArmSelfTestWatchdog(const TCHAR* Step);
	void UnlockPlayer();
	void ParkPawn();
	void UnparkPawn();
	void CurtainOut(float Duration);
	void CurtainIn(float Duration);
	void PrepareOpenLevel();
	void LogFloorState(const TCHAR* Label) const;
	void RunSelfTestStep();
	/** 셀프 테스트 모드: 콘솔 변수 soul.Floor.SelfTestMode, 비었으면 -SoulFloorSelfTestMode= */
	FString ReadSelfTestMode() const;
	/** result, shots가 아니면 이번 런을 명예의 전당에 내지 않게 한다. 제출하면 true */
	bool ApplySelfTestSubmitRule(const FString& Mode) const;
	void RunSelfTestFloors();
	void RunSelfTestMob(int32 Step);
	void RunSelfTestBoss(int32 Step);
	void RunSelfTestDeath(int32 Step);
	void RunSelfTestSpawn(int32 Step);
	void RunSelfTestBossFront(int32 Step);
	void RunSelfTestShots(int32 Step);
	TArray<TWeakObjectPtr<AActor>> SelfTestShotTargets;
	void RunSelfTestStats(int32 Step);
	void LogSkillState(const TCHAR* Label) const;
	void LogPawnPose(const TCHAR* Label) const;
	void ScheduleSelfTest(int32 Step, float Delay);
	void LogPlayerHealth(const TCHAR* Label) const;
	void FinishSelfTest();

	APlayerController* GetPlayerController() const;
	bool IsLastFloor() const;
	void SpawnFloorObjects(const FPMGResult& Result);
	void SpawnFloorMines(const FPMGResult& Result);
	void ApplyDescendHeal();

public:
	/** 이벤트 오브젝트가 효과를 낸 뒤 부른다 (세트 보너스) */
	void NotifyRoomEventUsed(ASoulRoomEventObject* Object);

protected:
	void RunSelfTestEvents(int32 Step);
	void RunSelfTestItems(int32 Step);
	void RunSelfTestWeapon(int32 Step);
	void RunSelfTestSummon(int32 Step);
	void RunSelfTestFPath(int32 Step);
	void DebugFace(const AActor* Target, float Distance, const FVector& FromDirection);
	void DebugPressPortal(int32 Tries);

	TWeakObjectPtr<APMGMapBuilder> Builder;
	TWeakObjectPtr<APawn> PlayerPawn;
	TWeakObjectPtr<ASoulPackedLevelActor> ExitRoom;
	TWeakObjectPtr<ASoulFloorPortal> Portal;

	UPROPERTY(Transient)
	TObjectPtr<USoulKeyHintWidget> MapKeyHint;
	TArray<TWeakObjectPtr<AActor>> FloorActors;
	TArray<TWeakObjectPtr<ASoulRoomEventObject>> FloorEventObjects;
	bool bEventSetBonusGiven = false;

	int32 CurrentFloorIndex = INDEX_NONE;
	int32 GenerateAttempt = 0;
	double GenerateStartTime = 0.0;
	double CurtainClosedTime = 0.0;
	double NavWaitStartTime = 0.0;
	double FloorReadyTime = 0.0;
	TMap<int32, double> RoomEnterTime;

	bool bFirstFloor = true;
	bool bTransitioning = false;
	bool bClearPending = false;
	bool bPlayerDead = false;
	bool bNavReady = false;
	bool bSelfTest = false;
	bool bDescendHealPending = false;
	FString SelfTestMode;
	FPMGResult LastResult;

	FTimerHandle CurtainTimer;
	FTimerHandle UnloadTimer;
	FTimerHandle NavTimer;
	FTimerHandle RevealTimer;
	FTimerHandle PortalTimer;
	FTimerHandle DeathTimer;
	FTimerHandle RestartTimer;
	FTimerHandle SelfTestTimer;
	FTimerHandle SelfTestWatchdog;
	FTimerHandle SelfTestHealTimer;
	TWeakObjectPtr<AActor> SelfTestPickup;

	UPROPERTY(Transient)
	TObjectPtr<class USoulResultWidget> ResultWidget;
	FTimerHandle SelfTestInputTimer;
	int32 SelfTestRepeat = 0;
};
