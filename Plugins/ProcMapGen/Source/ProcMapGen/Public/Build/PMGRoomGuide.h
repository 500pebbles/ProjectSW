#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/PMGTypes.h"
#include "PMGRoomGuide.generated.h"

class UPMGSettings;
class ULineBatchComponent;

/**
 * 방 에셋 가이드 (Roadmap 3-4절 "검증 도구", Phase 11). 방 레벨(또는 방 액터를 놓은 레벨)에 하나 놓으면,
 * CellSize 기준 방 영역과 빌더가 문/벽을 놓을 개구부 자리(경계의 셀 엣지 중점)를 선으로 그린다. 방 에셋이 규약(Roadmap 3-2절)에 맞는지 눈으로 비교하는 용도.
 *
 * - 이 액터의 위치 = 방 에셋의 피벗 (방 영역 바닥 중앙). 레벨 에셋은 레벨 원점이 피벗이므로 위치를 (0, 0, 0)에 둔다
 * - 그리는 것: 흰 상자 = 방 영역 (높이 GuideHeight), 회색 선 = 방 안 셀 경계, 초록 세로선 + 화살표 = 개구부 (화살표는 방 밖 방향), 노란 화살표 = 피벗과 +X
 * - 에디터 전용 액터: 패키징한 게임에는 들어가지 않고, 플레이 중(PIE)에는 그리지 않는다
 */
UCLASS()
class PROCMAPGEN_API APMGRoomGuide : public AActor
{
	GENERATED_BODY()

public:
	APMGRoomGuide();

	/** CellSize를 읽을 설정 에셋. None이면 설정의 기본 CellSize(1000)를 쓴다 */
	UPROPERTY(EditAnywhere, Category = "ProcMapGen")
	TObjectPtr<UPMGSettings> Settings;

	/** 확인할 방 모양. 2칸 에셋은 가로(X축 2칸)로 만들므로 세로 2칸은 고를 수 없다 */
	UPROPERTY(EditAnywhere, Category = "ProcMapGen", meta = (InvalidEnumValues = "DualVertical"))
	EPMGRoomShape Shape = EPMGRoomShape::Single;

	/** 방 영역 상자와 개구부 선의 높이 (벽 높이와 비교하는 용도. 생성에는 쓰지 않음) */
	UPROPERTY(EditAnywhere, Category = "ProcMapGen", meta = (ClampMin = "1.0", Units = "cm"))
	float GuideHeight = 300.f;

	/** 에디터에서 놓거나, 옮기거나, 프로퍼티를 바꾸면 불린다. 가이드 선을 다시 그린다 */
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	/** 가이드 선을 담는 컴포넌트. 선은 저장되지 않고 OnConstruction이 그릴 때마다 새로 만든다 */
	UPROPERTY()
	TObjectPtr<ULineBatchComponent> GuideLines;
};
