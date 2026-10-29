#pragma once

#include "CoreMinimal.h"
#include "Data/PMGTypes.h"
#include "PMGResult.generated.h"

/**
 * 생성 결과의 방 하나 (Roadmap 1-3절). Core의 FPMGRoom을 Blueprint에서 읽을 수 있게 옮긴 것이다 (PMG::GenerateMap이 채움)
 */
USTRUCT(BlueprintType)
struct PROCMAPGEN_API FPMGResultRoom
{
	GENERATED_BODY()

	/** 방 ID = FPMGResult::Rooms의 인덱스. 방의 왼쪽 위 셀 기준으로 위쪽 줄부터, 왼쪽부터 붙는다 */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	int32 Id = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	EPMGRoomShape Shape = EPMGRoomShape::Single;

	/** 방이 차지하는 셀 범위 [Min, Max). X = Col(월드 X축), Y = Row(월드 Y축) */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	FIntRect Bounds;

	/** 방 타입: Entrance, Exit, Standard, 또는 커스텀 타입 이름 */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	FName RoomType;

	/** 상하좌우로 맞닿은 다른 방의 ID (이웃 그래프). 오름차순 */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	TArray<int32> Neighbors;

	/** 사용(연결)되는 방인지 */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	bool bConnected = false;

	/** 추가방인지: 루트에 없고 루트 방 옆에 붙인 방. true면 bConnected도 true */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	bool bExtra = false;

	/**
	 * 배정된 방 에셋의 인덱스.
	 * 사용된 방(bConnected)은 이 방의 타입(RoomType)과 크기에 맞는 설정 에셋 리스트의 인덱스 (예: Standard 2칸 방이면 StandardRooms.DualRooms.Variants),
	 * 미사용 방은 설정의 FillerRooms 인덱스 (UnusedRoomPolicy가 SpawnFiller일 때만).
	 * -1이면 스폰하지 않는 방 (리스트가 비어 있는 사용된 방, 또는 채움 방을 쓰지 않는 미사용 방)
	 */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	int32 VariantIndex = INDEX_NONE;

	/** 에셋 회전: 0, 1, 2, 3 → 0/90/180/270°. 2칸 방은 0 또는 2 (세로 방을 세우는 90°는 따로 더한다, Roadmap 3-3절) */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	uint8 Rotation = 0;
};

/** 생성 결과의 문 하나. 맞닿은 두 셀 CellA(RoomA의 셀)와 CellB(RoomB의 셀) 사이의 경계에 놓인다 */
USTRUCT(BlueprintType)
struct PROCMAPGEN_API FPMGResultDoor
{
	GENERATED_BODY()

	/** 입구에 가까운 쪽 방: 루트 문은 루트에서 앞 방, 추가방 문은 루트 방 */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	int32 RoomA = INDEX_NONE;

	/** 들어가는 쪽 방: 루트 문은 루트에서 뒤 방, 추가방 문은 추가방 */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	int32 RoomB = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	FIntPoint CellA = FIntPoint::NoneValue;

	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	FIntPoint CellB = FIntPoint::NoneValue;
};

/**
 * 생성 결과의 커넥터 하나 (Roadmap 3-4절): 셀 경계 하나를 채우는 문 또는 벽. 맞닿은 두 셀 CellA와 CellB 사이의 경계에 놓인다.
 * 스폰되는 방이 하나라도 접한 경계마다 하나씩 있다
 */
USTRUCT(BlueprintType)
struct PROCMAPGEN_API FPMGResultConnector
{
	GENERATED_BODY()

	/** 경계 안쪽 셀 (항상 맵 안) */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	FIntPoint CellA = FIntPoint::NoneValue;

	/** 경계 건너편 셀 (맵 테두리면 맵 밖). 문이면 문의 CellA → CellB 방향이 들어가는 방향 (RoomA → RoomB) */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	FIntPoint CellB = FIntPoint::NoneValue;

	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	EPMGConnectorType Type = EPMGConnectorType::Wall;

	/** 문 에셋을 가져온 방 타입: 그 타입의 Doors. None이면 설정의 전역 Doors. 벽이면 None */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	FName DoorListRoomType;

	/** 배정된 에셋: 문은 DoorListRoomType의 문 리스트, 벽은 Walls의 인덱스. -1이면 리스트가 비어 있어 스폰하지 않음 (문이면 개구부만 열림) */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	int32 VariantIndex = INDEX_NONE;
};

/** 생성 결과 (PMG::GenerateMap). 실패하면 bSuccess = false, FailReason에 사유가 있고 방/문/커넥터/루트는 비어 있다 */
USTRUCT(BlueprintType)
struct PROCMAPGEN_API FPMGResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	bool bSuccess = false;

	/** 실패 사유. 설정 검증 실패면 검증 에러, 시도가 모두 실패하면 단계별 실패 횟수와 마지막 실패 사유 */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	FString FailReason;

	/** 시도 횟수. 성공하면 성공한 시도까지의 수, 시도가 모두 실패하면 MaxAttempts, 설정 검증 실패면 0 */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	int32 AttemptCount = 0;

	/** 이 결과를 만든 시드 (PMG::GenerateMap에 넘긴 값). 같은 설정에 이 시드를 다시 넣으면 같은 결과가 나온다 */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	int32 Seed = 0;

	/** 경고: 설정 검증의 무시(보정) 규칙 경고 + 성공한 시도의 방 에셋 배정 경고. 스폰(APMGMapBuilder)의 문제는 여기가 아니라 빌더의 SpawnErrors에 남는다 */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	TArray<FString> Warnings;

	/** 방 목록 (사용되지 않는 방 포함). 인덱스 = 방 ID */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	TArray<FPMGResultRoom> Rooms;

	/** 문 목록. 연결된 방 쌍마다 하나 */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	TArray<FPMGResultDoor> Doors;

	/** 커넥터 목록 (문/벽). 셀을 위쪽 줄부터, 왼쪽부터 본 순서 */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	TArray<FPMGResultConnector> Connectors;

	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	int32 EntranceRoomId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	int32 ExitRoomId = INDEX_NONE;

	/** 루트 1: 입구방 → 출구방 순서의 방 ID (양 끝 포함) */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	TArray<int32> PrimaryRoute;

	/** 루트 2: 루트 1을 피해 돌아가는 두 번째 경로. 형식은 PrimaryRoute와 같다 */
	UPROPERTY(BlueprintReadOnly, Category = "ProcMapGen")
	TArray<int32> SecondaryRoute;
};
