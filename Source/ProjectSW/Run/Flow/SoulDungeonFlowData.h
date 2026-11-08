#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulDungeonFlowData.generated.h"

class UPMGSettings;

/** 층 하나의 설정. 몹 규칙, 이벤트, 지뢰, 로딩 항목은 다음 단계에서 이 구조체에 붙는다 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulFloorDefinition
{
	GENERATED_BODY()

	/** 이 층의 맵 생성 설정 (DA_PMGSettings_Fn). 비어 있으면 그 층을 시작하지 못한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
	TObjectPtr<UPMGSettings> MapSettings;
};

/**
 * 던전 층 배열과 층 전환 값 (DA_DungeonFlow). BP_DungeonGameMode의 USoulFloorDirectorComponent가 읽는다.
 * 값은 전부 이 에셋에서 고친다. C++ 기본값은 0이다.
 */
UCLASS(BlueprintType)
class PROJECTSW_API USoulDungeonFlowData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 층 순서대로. 1층 = 0번 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floors")
	TArray<FSoulFloorDefinition> Floors;

	/** 맵을 지운(Clear) 뒤 다음 층 Generate까지 기다리는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0", Units = "s"))
	float UnloadHoldSec = 0.f;

	/** 생성이 끝난 뒤 NavMesh 재빌드를 기다리는 최대 시간. 넘으면 그냥 연다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0", Units = "s"))
	float NavWaitTimeoutSec = 0.f;

	/** NavMesh 빌드 여부를 확인하는 간격 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0", Units = "s"))
	float NavPollIntervalSec = 0.f;

	/** 빌더의 프레임당 스폰 예산. 0이면 L_Dungeon 빌더에 놓인 값을 그대로 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0", Units = "ms"))
	float SpawnTimeBudgetMs = 0.f;

	/** 폰 도착 위치 = 입구 방 중심 + ArrivalOffset (+ ArrivalDropHeight 위) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	FVector ArrivalOffset = FVector::ZeroVector;

	/** 도착할 때 떨어지는 높이 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0", Units = "cm"))
	float ArrivalDropHeight = 0.f;

	/** 생성 실패 시 Seed+1로 다시 시도하는 횟수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0"))
	int32 GenerateRetryCount = 0;
};
