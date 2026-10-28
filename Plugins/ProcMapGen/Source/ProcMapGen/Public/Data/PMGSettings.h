#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/PMGTypes.h"
#include "PMGSettings.generated.h"

namespace PMG
{
	constexpr int32 DefaultMaxAttempts = 50;
}

/** UPMGSettings::Validate() 결과. 보정된 값과 파생 값을 함께 담는다 */
struct PROCMAPGEN_API FPMGValidationResult
{
	/** 즉시 실패 조건이 없으면 true */
	bool bValid = false;

	/** 즉시 실패 사유 (여러 개면 모두 나열) */
	TArray<FString> Errors;

	/** 무시(보정) 규칙이 적용된 경고 */
	TArray<FString> Warnings;

	// 보정된 값
	int32 TotalRoomCount = 0;
	int32 MaxAttempts = PMG::DefaultMaxAttempts;
	int32 RouteCostPenalty = 0;
	/** 입구 방이 될 수 있는 크기 (에셋이 등록된 크기) */
	FPMGRoomSizeSet EntranceRoomSizes;
	/** 출구 방이 될 수 있는 크기 (에셋이 등록된 크기) */
	FPMGRoomSizeSet ExitRoomSizes;
	/** 입구/출구가 될 수 있는 (입구 크기, 출구 크기) 조합 (PMG::MakeEntranceExitSizeCombos). 비어 있으면 즉시 실패 */
	TArray<FPMGEntranceExitSizes> EntranceExitSizeCombos;
	/** 유효한 CustomTypes 인덱스 (정의 순서) */
	TArray<int32> ValidCustomTypeIndices;
	/**
	 * 추가방용으로 예약할 큰 방마다 고를 수 있는 크기 (2-9절, PMG::ChooseReservedBigRooms의 입력).
	 * bExtraRoom 타입 중 1칸 에셋이 없는 타입의 Count만큼, 그 타입에 에셋이 있는 큰 크기(bDual/bQuad)를 담는다 (정의 순서)
	 */
	TArray<FPMGRoomSizeSet> ReservedRoomSizeChoices;

	// 파생 값
	/** 큰 방을 모두 만든 뒤 맵에 남는 1칸 방 수 */
	int32 SingleRoomCount = 0;
	/** 큰 방을 모두 만든 뒤 맵의 모든 방 수 (사용 여부와 무관) */
	int32 AllRoomCount = 0;
	/** 사용되는 1칸 방 수 */
	int32 UsedSingleRoomCount = 0;
	/** QuadRoomCount 최대값 (PMG::GetMaxQuadRoomCount) */
	int32 MaxQuadRoomCount = 0;
	/** 현재 QuadRoomCount 기준 DualRoomCount 최대값 (PMG::GetMaxDualRoomCount) */
	int32 MaxDualRoomCount = 0;
	/** 루트에 필요한 방 수 추정 (휴리스틱, 입구/출구 크기 조합 중 가장 큰 값) */
	int32 EstimatedRouteRoomCount = 0;
	int32 RecommendedTotalRoomCount = 0;

	FString GetFailReason() const { return FString::Join(Errors, TEXT("; ")); }
};

/** 절차적 맵 생성 설정 */
UCLASS(BlueprintType)
class PROCMAPGEN_API UPMGSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	// ---------- Map ----------

	/** 월드 Y축 칸 수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ClampMin = "1", ClampMax = "100"))
	int32 Rows = 10;

	/** 월드 X축 칸 수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ClampMin = "1", ClampMax = "100"))
	int32 Cols = 10;

	/**
	 * 기본 시드. 빌더(APMGMapBuilder)가 BeginPlay에서 생성할 때 쓴다. 0도 다른 값과 같은 보통 시드다 (무작위가 아님).
	 * 플레이할 때마다 다른 맵을 원하면 빌더의 Use Random Seed를 켠다
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	int32 Seed = 0;

	/** 생성 최대 시도 횟수. 0 이하면 기본값(50) 사용 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	int32 MaxAttempts = PMG::DefaultMaxAttempts;

	/** 셀 1칸의 월드 크기 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ClampMin = "1.0", Units = "cm"))
	float CellSize = 1000.f;

	// ---------- Rooms ----------

	/** 4칸 방(2x2) 개수. 전부 사용됨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rooms", meta = (ClampMin = "0"))
	int32 QuadRoomCount = 2;

	/** 2칸 방 개수 (가로/세로 합산). 전부 사용됨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rooms", meta = (ClampMin = "0"))
	int32 DualRoomCount = 4;

	/** 연결되어 사용되는 방의 총 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rooms", meta = (ClampMin = "0"))
	int32 TotalRoomCount = 20;

	/** 입구방~출구방 최소 거리 (셀 단위 방 중심 맨해튼 거리, 루트가 지나는 방 수 아님). 루트에 큰 방을 만들 칸을 남기려고 실제 거리는 더 커질 수 있다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rooms", meta = (ClampMin = "0"))
	int32 MinEntranceExitDistance = 5;

	// ---------- Route ----------

	/** 두 번째 루트 계산 시 루트 1에 포함된 방의 비용 (다른 방의 비용은 1이므로 1보다 커야 함) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
	int32 RouteCostPenalty = 10;

	// ---------- Assets ----------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	FPMGRoomTypeAssets EntranceRooms;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	FPMGRoomTypeAssets ExitRooms;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	FPMGRoomTypeAssets StandardRooms;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets", meta = (TitleProperty = "TypeName"))
	TArray<FPMGCustomRoomType> CustomTypes;

	/** 기본 문 에셋. 방 타입의 Doors가 비어 있으면 이 리스트를 씀. 모두 비어 있으면 개구부만 열림 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	FPMGAssetSet Doors;

	/** 벽(개구부 막이) 에셋. 비어 있으면 연결되지 않은 개구부가 열린 채로 남음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	FPMGAssetSet Walls;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	EPMGUnusedRoomPolicy UnusedRoomPolicy = EPMGUnusedRoomPolicy::DontSpawn;

	/** 미사용 방(1x1)에 스폰할 채움 에셋. 미사용 방마다 이 리스트에서 하나를 고른다 (방 에셋과 같은 셔플 백, 회전) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets", meta = (EditCondition = "UnusedRoomPolicy == EPMGUnusedRoomPolicy::SpawnFiller"))
	FPMGAssetSet FillerRooms;

	/** 입구방 중앙(바닥 높이)에 스폰할 액터 (예: PlayerStart 계열). None이면 스폰하지 않음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	TSoftClassPtr<AActor> EntranceMarkerClass;

	/** 출구방 중앙(바닥 높이)에 스폰할 액터. None이면 스폰하지 않음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	TSoftClassPtr<AActor> ExitMarkerClass;

	// ---------- Advanced ----------

	/**
	 * 큰 방 자리를 고를 때 뽑는 후보 수 (best-candidate). 고른 자리는 대략 거리 순위 상위 1/(값+1)에 든다 (1 = 무작위, 8 ≈ 상위 11%, 16 ≈ 상위 6%).
	 * 클수록 큰 방이 고르게 퍼지지만 시드마다 배치가 비슷해진다. 보통 1~16.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Advanced", meta = (ClampMin = "1", UIMin = "1", UIMax = "16"))
	int32 SpreadCandidateCount = 8;

	// ---------- Info (읽기 전용) ----------

	/** 큰 방을 모두 만든 뒤 맵에 남는 1칸 방 수 = Rows*Cols - 4*QuadRoomCount - 2*DualRoomCount */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Info")
	int32 SingleRoomCount = 0;

	/** 사용되는 1칸 방 수 = TotalRoomCount - QuadRoomCount - DualRoomCount */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Info")
	int32 UsedSingleRoomCount = 0;

	/** 큰 방을 모두 만든 뒤 맵의 모든 방 수 = QuadRoomCount + DualRoomCount + SingleRoomCount (사용 여부와 무관) */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Info")
	int32 AllRoomCount = 0;

	/** QuadRoomCount 최대값 = floor(Rows/2) * floor(Cols/2) */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Info")
	int32 MaxQuadRoomCount = 0;

	/** 현재 QuadRoomCount 기준 DualRoomCount 최대값 = floor((Rows*Cols - 4*QuadRoomCount) / 2) */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Info")
	int32 MaxDualRoomCount = 0;

	/** 루트에 필요한 방 수 추정 (휴리스틱) */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Info")
	int32 EstimatedRouteRoomCount = 0;

	/** 권장 TotalRoomCount (추정) */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Info")
	int32 RecommendedTotalRoomCount = 0;

	/** 검증 결과 요약 */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Info")
	FString ValidationStatus;

	/** 검증 메시지 (에러/경고) */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Info")
	TArray<FString> ValidationMessages;

public:
	/** 1-2절 규칙으로 설정을 검증한다. 로그는 남기지 않는다 */
	FPMGValidationResult Validate() const;

	/** Info 항목을 갱신한다 */
	void RefreshInfo(bool bLogMessages);

	virtual void PostInitProperties() override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	/**
	 * 모든 에셋 리스트 항목의 bCanRotate를 채운다.
	 * 방 에셋(방 타입의 크기별 리스트, FillerRooms)은 true, 문/벽(전역 Doors/Walls, 방 타입마다의 Doors)은 false + bAllowRotation도 끔
	 */
	void RefreshCanRotate();
};
