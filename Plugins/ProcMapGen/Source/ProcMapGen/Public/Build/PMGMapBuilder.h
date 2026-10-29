#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/PMGResult.h"
#include "Engine/StreamableManager.h"
#include "PMGMapBuilder.generated.h"

class UPMGSettings;
class ULevelStreamingDynamic;
class ULineBatchComponent;

/** 빌더가 지금 하는 일 (APMGMapBuilder::GetBuildState). 로딩 화면에 "불러오는 중" 같은 글자를 보여 줄 때 쓴다 */
UENUM(BlueprintType)
enum class EPMGBuildState : uint8
{
	/** 생성 중이 아님 (끝났거나 Generate를 부르지 않음) */
	Idle,
	/** 앞의 생성에서 로드한 레벨이 월드에서 빠지기를 기다리는 중 */
	WaitingForUnload,
	/** 스폰할 액터 클래스를 비동기로 불러오는 중 */
	LoadingClasses,
	/** 프레임마다 나눠 스폰하는 중 */
	Spawning,
	/** 스폰은 끝났고 방 레벨이 로드되어 보이기를 기다리는 중 */
	LoadingLevels
};

/**
 * 생성이 끝났을 때 (APMGMapBuilder::OnGenerationFinished). Result는 맵 생성 결과(실패하면 bSuccess = false, FailReason),
 * SpawnErrors는 스폰하지 못한 방/문/벽/마커의 이유 (비어 있으면 모두 스폰됨)
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPMGGenerationFinishedSignature, const FPMGResult&, Result, const TArray<FString>&, SpawnErrors);

namespace PMG
{
	/**
	 * 방을 스폰할 월드 Transform (Roadmap 3-3절). Origin은 맵의 (0, 0) 셀 모서리 (빌더 액터의 위치).
	 * 위치 = Origin + 방 영역 중심(셀 단위) x CellSize, 높이는 Origin 그대로 (방 에셋의 피벗은 방 영역 바닥 중앙).
	 * Yaw = (세로 2칸 방이면 90°) + Rotation x 90°. 2칸 에셋은 가로(X축 2칸)로 만들므로 세로 방은 90° 세워서 놓는다
	 */
	PROCMAPGEN_API FTransform GetRoomSpawnTransform(const FPMGResultRoom& Room, float CellSize, const FVector& Origin);

	/**
	 * 커넥터(문/벽)를 스폰할 월드 Transform (Roadmap 3-4절). 위치 = CellA와 CellB 사이 경계의 중점 (두 셀 중심의 가운데), 높이는 Origin 그대로.
	 * Yaw = CellA → CellB 방향 (+X 0°, +Y 90°, -X 180°, -Y 270°). 문 에셋의 +X가 RoomA → RoomB를 향한다
	 */
	PROCMAPGEN_API FTransform GetConnectorSpawnTransform(const FPMGResultConnector& Connector, float CellSize, const FVector& Origin);

	/**
	 * Shape 모양 방의 개구부 자리 (Roadmap 3-2절: 경계의 셀 엣지마다 하나, 1칸 4개 / 2칸 6개 / 4칸 8개). RoomCenter는 방 영역 바닥 중앙 (방 에셋의 피벗).
	 * 빌더가 문/벽을 놓는 자리(GetConnectorSpawnTransform)와 같은 식으로 구한다. Yaw는 방 안 → 밖 방향.
	 * 순서: 셀을 위쪽 줄부터, 왼쪽부터 보고 셀마다 -Y, +X, +Y, -X 쪽 경계. 방 에셋 가이드(APMGRoomGuide)가 쓴다
	 */
	PROCMAPGEN_API TArray<FTransform> GetRoomOpeningTransforms(EPMGRoomShape Shape, float CellSize, const FVector& RoomCenter);
}

/**
 * 레벨 빌더 (Roadmap 3-6절, Phase 10). 레벨에 하나 놓고 Settings를 지정하면 맵을 생성해(PMG::GenerateMap) 결과대로 방/문/벽/입구·출구 마커를 스폰한다.
 *
 * - 에셋 항목마다 ActorClass가 지정되어 있으면 액터 스폰, Level이 지정되어 있으면 동적 레벨 스트리밍으로 로드 (둘 중 하나만 지정해야 함)
 * - 맵의 (0, 0) 셀 모서리 = 이 액터의 위치. 액터의 회전과 스케일은 쓰지 않는다
 * - 스폰한 액터와 스트리밍 레벨은 이 액터가 가지고 있다가 Clear()에서 정리한다. 이름은 "<빌더 이름>_Room_<방 ID>"처럼 정해져 있다
 *   (같은 시드면 서버와 클라이언트에서 이름이 같다, Roadmap 3-6절)
 * - 레벨 로드는 비동기라, 모든 레벨이 보이게 되거나 로드에 실패한 뒤 OnGenerationFinished가 온다.
 *   레벨이 없어도 완료 이벤트는 Generate()를 부른 다음 프레임의 Tick에서 온다 (BeginPlay 순서와 상관없이 바인딩할 수 있게)
 * - 레벨을 내리는 데도 몇 프레임이 걸린다. 앞의 생성에서 로드한 레벨이 아직 월드에 남아 있으면, 그 레벨이 빠진 뒤에 스폰한다
 *   (같은 이름의 레벨이 남아 있으면 엔진이 로드를 거부하므로)
 * - 스폰은 프레임마다 SpawnTimeBudgetMs만큼씩 나눠 하고, 처음 쓰는 액터 클래스는 그 전에 비동기로 불러온다 (Phase 12).
 *   그동안 화면이 멈추지 않으므로 GetBuildProgress()로 로딩 바를 그릴 수 있다
 * - 네트워크 코드는 없다. 스폰은 이 머신에서만 하고 리플리케이트하지 않는다 (Roadmap 3-6절 싱글/멀티 중립)
 * - 에디터에서는 Details 패널의 Preview 버튼으로 맵 구조를 선으로만 미리 볼 수 있다 (Phase 11). 미리보기는 아무것도 스폰하지 않는다
 */
UCLASS()
class PROCMAPGEN_API APMGMapBuilder : public AActor
{
	GENERATED_BODY()

public:
	APMGMapBuilder();

	/** 맵 생성 설정 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen")
	TObjectPtr<UPMGSettings> Settings;

	/** 켜져 있으면 BeginPlay에서 Settings의 Seed로 Generate()를 부른다. 시드를 직접 정하려면 끄고 Generate(Seed)를 부른다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen")
	bool bGenerateOnBeginPlay = true;

	/**
	 * 켜져 있으면 BeginPlay에서 Settings의 Seed 대신 무작위 시드로 생성한다 (플레이할 때마다 다른 맵).
	 * 쓴 시드는 Result.Seed와 로그에 남으므로, 마음에 드는 맵은 그 값을 Settings의 Seed에 넣고 이 옵션을 끄면 다시 볼 수 있다.
	 * 각 머신이 따로 시드를 고르므로 멀티플레이에서는 끄고, 서버가 정한 시드로 Generate(Seed)를 부른다
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen", meta = (EditCondition = "bGenerateOnBeginPlay"))
	bool bUseRandomSeed = false;

	/**
	 * 한 프레임에 스폰하는 데 쓸 시간 (ms). 넘으면 나머지는 다음 프레임에 스폰한다 (프레임마다 최소 1개). 처음 쓰는 액터 클래스는 스폰을 시작하기 전에 비동기로 불러온다.
	 * 작을수록 프레임이 덜 멈추지만 다 스폰하기까지 오래 걸린다. 0이면 Generate()를 부른 프레임에 모두 스폰한다 (클래스도 그 자리에서 불러옴)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen", meta = (ClampMin = "0", Units = "ms"))
	float SpawnTimeBudgetMs = 10.f;

	/** 생성이 끝났을 때 (모든 레벨 로드가 끝난 뒤). 생성에 실패해도 온다 */
	UPROPERTY(BlueprintAssignable, Category = "ProcMapGen")
	FPMGGenerationFinishedSignature OnGenerationFinished;

	/**
	 * 마지막 Generate()의 맵 생성 결과. Clear()하면 비워진다.
	 * Details 패널에는 보이지 않는다 (FPMGResult의 멤버는 Blueprint 전용이라 펼쳐도 내용이 없음). Blueprint나 완료 이벤트로 읽는다
	 */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "ProcMapGen")
	FPMGResult Result;

	/** 마지막 Generate()에서 스폰하지 못한 것의 이유 (에셋 항목이 비어 있음, 클래스를 불러오지 못함, 레벨 로드 실패 등) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "ProcMapGen")
	TArray<FString> SpawnErrors;

	/** 스폰한 액터 (방, 문, 벽, 채움 방, 마커). 스폰한 순서 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "ProcMapGen")
	TArray<TObjectPtr<AActor>> SpawnedActors;

	/** 로드한 스트리밍 레벨 (Level이 지정된 에셋). 스폰한 순서 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "ProcMapGen")
	TArray<TObjectPtr<ULevelStreamingDynamic>> SpawnedLevels;

	/**
	 * 지금 만든 것을 모두 지우고(Clear) Settings와 Seed로 맵을 새로 생성해 스폰한다. 같은 Settings + 같은 Seed면 항상 같은 맵이 된다.
	 * 생성에 실패하면 아무것도 스폰하지 않는다. 끝나면 OnGenerationFinished (다음 프레임 이후).
	 * 앞의 생성에서 로드한 레벨이 아직 내려가는 중이면 다 내려간 뒤에, 불러올 액터 클래스가 있으면 다 불러온 뒤에 스폰을 시작한다 (이후의 Tick).
	 * 아니면 이 함수 안에서 바로 스폰을 시작해 SpawnTimeBudgetMs만큼 스폰하고, 나머지는 다음 프레임들에 스폰한다
	 */
	UFUNCTION(BlueprintCallable, Category = "ProcMapGen")
	void Generate(int32 Seed);

	/**
	 * 스폰한 액터를 모두 지우고 로드한 레벨을 모두 내린다 (레벨은 몇 프레임 뒤에 월드에서 빠진다). 결과(Result, SpawnErrors)도 비운다.
	 * 기다리던 완료 이벤트는 오지 않는다
	 */
	UFUNCTION(BlueprintCallable, Category = "ProcMapGen")
	void Clear();

	/** Generate()를 부른 뒤 아직 완료 이벤트를 보내지 않았는지 (앞의 레벨이 내려가기를, 클래스 불러오기를, 스폰을, 또는 레벨 로드를 기다리는 중) */
	UFUNCTION(BlueprintPure, Category = "ProcMapGen")
	bool IsGenerating() const { return bGenerating; }

	/**
	 * 스폰 진행도 (로딩 바용). 0 ~ 1 = OutCompletedCount / OutTotalCount, OutTotalCount가 0이면 0.
	 * OutTotalCount = 이번 생성에서 스폰할 것 전체 (방, 채움 방, 문/벽, 마커). 맵을 생성한 직후(Generate 안)에 정해진다.
	 * OutCompletedCount = 끝난 것: 액터는 스폰했을 때, 레벨은 로드되어 보이게 됐을 때(또는 로드에 실패했을 때). 스폰에 실패한 것도 끝난 것으로 센다.
	 * 완료 이벤트 뒤에는 1이고 Clear()하면 0. 문/벽이 방보다 훨씬 많으므로 로딩 바에는 이 값을 쓴다
	 */
	UFUNCTION(BlueprintPure, Category = "ProcMapGen")
	float GetBuildProgress(int32& OutCompletedCount, int32& OutTotalCount) const;

	/** 방(사용된 방 + 채움 방)만 센 진행도. "방 120/225"처럼 글자로 보여 줄 때. 세는 방법은 GetBuildProgress와 같다 */
	UFUNCTION(BlueprintPure, Category = "ProcMapGen")
	void GetRoomBuildProgress(int32& OutCompletedRoomCount, int32& OutTotalRoomCount) const;

	/** 빌더가 지금 하는 일 (생성 중이 아니면 Idle) */
	UFUNCTION(BlueprintPure, Category = "ProcMapGen")
	EPMGBuildState GetBuildState() const;

	virtual void Tick(float DeltaSeconds) override;

	// ---------- 구조 미리보기 (에디터 버튼, Phase 11) ----------

	/**
	 * 마지막 구조 미리보기에 쓴 시드. 이 맵을 플레이에서 쓰려면 이 값을 Settings의 Seed에 넣는다.
	 * 미리보기를 지워도(Clear Preview) 남는다
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Preview")
	int32 PreviewSeed = 0;

	/**
	 * 마지막 구조 미리보기의 생성 결과. Generate()의 Result와 따로 둔다.
	 * 버튼과 테스트만 쓰고 GC할 오브젝트 포인터도 없어 UPROPERTY가 아니다 (실패 사유는 Output Log에 나옴)
	 */
	FPMGResult PreviewResult;

	/**
	 * Settings의 Seed로 맵을 생성해 구조를 선으로 그린다 (스폰 없음). 플레이할 때 BeginPlay가 만드는 맵과 같다 (Use Random Seed가 꺼져 있을 때).
	 * 흰 테두리 = 사용된 방 (회색 = 미사용 방), 이중선 = 추가방, 방 가운데 흰 원 = 입구, 흰 X표 = 출구, 색 상자 = 커스텀 타입 (색은 로그에 나옴),
	 * 주황 채운 화살표 = 문 (경계에 걸쳐 RoomA → RoomB), 분홍 선 = 루트 1, 하늘색 선 = 루트 2
	 */
	UFUNCTION(CallInEditor, Category = "Preview")
	void PreviewLayout();

	/** 무작위 시드로 미리 본다. 설정 에셋은 바꾸지 않는다 (쓴 시드는 PreviewSeed와 로그에 남음) */
	UFUNCTION(CallInEditor, Category = "Preview")
	void PreviewRandomSeed();

	/** 미리보기 선과 결과를 지운다 */
	UFUNCTION(CallInEditor, Category = "Preview")
	void ClearPreview();

	/** 에디터에서 빌더를 옮기거나 프로퍼티를 바꾸면 불린다. 미리보기를 지금 위치와 Settings로 다시 그린다 */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** 스폰할 것 하나: 에셋 항목과 놓을 자리 (Generate에서 결과로 만든다) */
	struct FSpawnRequest
	{
		/** 스폰할 에셋 항목. 마커는 ActorClass만 지정한 항목으로 담는다 */
		FPMGAssetVariant Variant;
		FTransform Transform;
		/** 액터 이름 / 스트리밍 레벨 이름 (예: "<빌더 이름>_Room_12") */
		FString ObjectName;
		/** 문제가 있을 때 SpawnErrors에 남길 대상 설명 (예: "방 12 (Standard)") */
		FString Label;
		/** 방(사용된 방, 채움 방)인지. 방 진행도(GetRoomBuildProgress)를 셀 때 쓴다 */
		bool bRoom = false;
	};

	/** Generate() 뒤 완료 이벤트를 아직 보내지 않았으면 true. Tick이 스폰과 레벨 로드가 끝났는지 본다 */
	bool bGenerating = false;

	/**
	 * 이번 생성에서 스폰할 목록 (스폰 순서). SpawnRequests[NextSpawnIndex]부터가 아직 스폰하지 않은 것이다.
	 * Generate 때의 Settings로 만들어 두므로 그 사이 Settings가 바뀌어도 결과와 어긋나지 않는다 (Soft 포인터뿐이라 UPROPERTY가 아니어도 됨).
	 * 다 스폰한 뒤에도 Clear()까지 남겨 두어 진행도의 전체 수로 쓴다
	 */
	TArray<FSpawnRequest> SpawnRequests;

	/** 다음에 스폰할 SpawnRequests의 인덱스 = 스폰을 끝낸 수 (레벨은 로드 요청까지) */
	int32 NextSpawnIndex = 0;

	/** SpawnRequests 중 방의 수, 그중 스폰을 끝낸 수 (방 진행도) */
	int32 TotalRoomSpawnCount = 0;
	int32 ProcessedRoomSpawnCount = 0;

	/** SpawnedLevels와 같은 순서로, 그 레벨이 방인지 (방 진행도에서 아직 로드 중인 방 레벨을 빼려고) */
	TArray<bool> SpawnedLevelIsRoom;

	/** 스폰하기 전에 액터 클래스를 비동기로 불러오는 요청. 불러온 클래스가 스폰 전에 가비지 컬렉션되지 않도록 완료(또는 Clear)까지 가지고 있는다 */
	TSharedPtr<FStreamableHandle> ClassLoadHandle;

	/** Clear()로 내리기를 요청했지만 아직 월드에 남아 있을 수 있는 레벨. 이 레벨들이 월드에서 빠져야 다음 생성을 스폰한다 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULevelStreamingDynamic>> UnloadingLevels;

	/** 미리보기 선을 담는 컴포넌트. 선은 저장되지 않는다 (레벨을 다시 열면 사라짐) */
	UPROPERTY()
	TObjectPtr<ULineBatchComponent> PreviewLines;

	/** Seed로 맵을 생성해(PreviewSeed, PreviewResult) 미리보기 선을 그린다 (Preview Layout, Preview Random Seed) */
	void PreviewWithSeed(int32 Seed);

	/** 미리보기 선을 지우고 PreviewResult가 성공한 결과면 다시 그린다 */
	void DrawPreview();

	/** 무작위 시드 (Use Random Seed, Preview Random Seed). 0 이상 */
	static int32 MakeRandomSeed();

	/** 스폰한 액터를 지우고 로드한 레벨의 내리기를 요청한 뒤 두 목록을 비운다 (Clear, 빌더가 지워질 때) */
	void ReleaseSpawned();

	/** 결과의 방 → 커넥터 → 입구·출구 마커를 스폰 목록(SpawnRequests)으로 만든다 (Generate에서, 생성에 성공했을 때) */
	void MakeSpawnRequests();

	/** 스폰 목록의 액터 클래스 중 아직 메모리에 없는 것을 비동기로 불러오기 시작한다 (ClassLoadHandle). 모두 있으면 아무것도 하지 않는다 */
	void StartClassLoading();

	/** 앞의 생성에서 내리기를 요청한 레벨 중 아직 월드에 남은 것이 있는지 */
	bool HasUnloadingLevels() const;

	/** 액터 클래스를 불러오는 중인지 */
	bool IsLoadingClasses() const;

	/**
	 * 내리던 레벨이 모두 빠졌고 클래스도 다 불러왔으면, 남은 스폰 목록을 SpawnTimeBudgetMs만큼 스폰한다 (최소 1개, 0이면 모두).
	 * 나머지는 다음 Tick에서 (Generate, Tick)
	 */
	void ContinueSpawning();

	/** 에셋 항목 하나를 스폰한다: ActorClass면 액터 스폰(SpawnedActors), Level이면 레벨 로드 시작(SpawnedLevels) */
	void SpawnRequest(const FSpawnRequest& Request);

	/** 스폰한 레벨 중 아직 로드 중인 것의 수 (보이게 되거나 실패하면 끝남). bRoomsOnly면 방 레벨만 */
	int32 CountLoadingLevels(bool bRoomsOnly) const;

	/** 완료: 로드에 실패한 레벨을 SpawnErrors에 남기고 OnGenerationFinished를 보낸다 */
	void FinishGeneration();
};
