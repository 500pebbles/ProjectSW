#pragma once

#include "CoreMinimal.h"
#include "Data/PMGResult.h"

class UPMGSettings;
struct FPMGLayout;
struct FPMGAssetLists;
struct FPMGValidationResult;

namespace PMG
{
	/**
	 * 시도 번호 AttemptIndex(0부터)에서 쓸 난수 스트림의 시드 (Roadmap 2-5절 Derive(Seed, attempt)).
	 * 첫 시도는 Seed 그대로이고, 다음 시도부터는 시도마다 0x9E3779B9(= 2^32 / 황금비)씩 더한다 (부호 없는 32비트로 넘치면 0부터 다시).
	 * 단순히 1씩 더하면 "Seed 5의 둘째 시도 = Seed 6의 첫 시도"가 되어, 첫 시도가 실패한 시드가 이웃 시드와 같은 맵을 만든다.
	 * 큰 수를 더하면 가까운 시드끼리 시도 시드가 겹치지 않는다. 정수 덧셈·곱셈뿐이라 어느 머신에서든 같다.
	 */
	PROCMAPGEN_API int32 GetAttemptSeed(int32 Seed, int32 AttemptIndex);

	/**
	 * 최종 검증 (2-6절 8단계). 한 시도의 모든 단계가 끝난 Layout이 맞는지 검사한다. 틀리면 첫 문제를 OutError에 쓰고 false (시도 실패).
	 * 1) 방 구조: 맵의 4칸/2칸 방 수 = QuadRoomCount/DualRoomCount, 면적과 CellToRoom 일치 (PMG::VerifyLayout)
	 * 2) 사용된 방: 큰 방은 모두 사용되고, 사용된(bConnected) 방 수 = TotalRoomCount
	 *    → 사용된 방의 크기별 개수 = QuadRoomCount / DualRoomCount / TotalRoomCount - QuadRoomCount - DualRoomCount
	 * 3) 연결성: 입구방에서 문만 따라가면 사용된 방 모두에, 그리고 사용된 방에만 도착하고, 출구방도 그중 하나
	 * 앞 단계들이 올바르면 항상 통과한다. 단계 함수의 버그를 결과로 내보내지 않기 위한 마지막 확인이다.
	 */
	PROCMAPGEN_API bool VerifyGeneratedLayout(const FPMGLayout& Layout, int32 QuadRoomCount, int32 DualRoomCount, int32 TotalRoomCount, FString& OutError);

	/**
	 * 에셋 배정(Phase 9, 10)의 입력: 설정 에셋의 Entrance/Exit/StandardRooms, 유효한 커스텀 타입(Validation.ValidCustomTypeIndices 순서), Doors, Walls,
	 * 그리고 UnusedRoomPolicy가 SpawnFiller면 FillerRooms를 옮겨 담는다. Validation은 Settings.Validate()의 결과다.
	 * 빌더(APMGMapBuilder)도 결과의 에셋 인덱스로 에셋을 찾을 때 이 함수로 같은 리스트를 만든다
	 */
	PROCMAPGEN_API FPMGAssetLists MakeAssetLists(const UPMGSettings& Settings, const FPMGValidationResult& Validation);

	/**
	 * Phase 8, 9: 설정과 시드로 맵 구조를 생성하고 에셋을 배정한다 (Roadmap 2-6절 시도 루프). 같은 설정 + 같은 Seed면 어느 머신에서든 같은 결과가 나온다.
	 * Seed는 호출할 때 정한다 (설정 에셋의 Seed는 쓰지 않는다. 에셋 값을 쓰려면 Settings.Seed를 넘긴다).
	 *
	 * 1. Settings.Validate(): 무시(보정) 규칙의 경고는 로그 + Warnings, 즉시 실패면 시도 없이 실패 (AttemptCount 0)
	 * 2. 시도마다 스트림 하나(GetAttemptSeed)를 모든 단계가 이어 쓴다:
	 *    모든 셀 1칸 → 이웃 → 입구/출구 크기 조합 → 추가방 예약 → 목표 거리 → 입구/출구 → 루트 1, 2 → 루트 위 큰 방 → 루트 연결
	 *    → 루트 옆 큰 방 → 분기 채우기 → 방 타입 배정 → 최종 검증 (VerifyGeneratedLayout)
	 *    어느 단계든 실패하면 그 단계의 실패 횟수를 세고 다음 시도로 넘어간다
	 * 3. 성공하면 그 시도의 스트림을 이어 써서 에셋을 배정한다 (Roadmap 2-5절): 방 에셋 → 커넥터 목록 → 문/벽 에셋 → 채움 방 에셋 (Core/PMGAssets.h).
	 *    구조가 모두 정해진 뒤라 에셋 리스트의 내용(개수, 회전 허용)을 바꿔도 맵 구조는 바뀌지 않는다
	 * 4. Layout을 결과(Rooms, Doors, Connectors, 입구/출구, 루트)에 옮기고, 그 시도의 에셋 경고를 로그 + Warnings에 남긴다
	 *    ("같은 에셋이 반복됨" 정보는 Log 수준 로그만).
	 *    MaxAttempts번 모두 실패하면 FailReason = 단계별 실패 횟수 + 마지막 실패 사유 (에러 로그도 남김)
	 */
	PROCMAPGEN_API FPMGResult GenerateMap(const UPMGSettings& Settings, int32 Seed);
}
