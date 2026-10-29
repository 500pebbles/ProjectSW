#pragma once

#include "CoreMinimal.h"
#include "Core/PMGLayout.h"

/**
 * 에셋 배정(Phase 9)에 쓰는 에셋 리스트. 설정 에셋(UPMGSettings)의 Assets 항목을 옮겨 담는다 (PMG::MakeAssetLists).
 * Core가 설정 에셋 헤더(UDataAsset)에 의존하지 않도록 따로 받는다 (Phase 7 PMG::AssignCustomRoomTypes와 같은 기준)
 */
struct PROCMAPGEN_API FPMGAssetLists
{
	FPMGRoomTypeAssets EntranceRooms;
	FPMGRoomTypeAssets ExitRooms;
	FPMGRoomTypeAssets StandardRooms;

	/** 유효한 커스텀 타입만 (UPMGSettings::Validate()의 ValidCustomTypeIndices 순서). 이름이 서로 다르다 */
	TArray<FPMGCustomRoomType> CustomTypes;

	/** 전역 문 리스트 (방 타입의 Doors가 비어 있을 때 씀) */
	FPMGAssetSet Doors;

	/** 벽(개구부 막이) 리스트 */
	FPMGAssetSet Walls;

	/** 미사용 방에 스폰할 채움 방 리스트. 설정의 UnusedRoomPolicy가 SpawnFiller일 때만 설정의 FillerRooms이고, 아니면 비어 있다 (PMG::MakeAssetLists) */
	FPMGAssetSet FillerRooms;

	/** 방 타입 이름(Entrance, Exit, Standard, 커스텀 타입 이름)의 에셋. 없는 이름이면 nullptr */
	const FPMGRoomTypeAssets* FindRoomTypeAssets(FName RoomType) const;
};

namespace PMG
{
	/**
	 * Phase 9 ① 방 에셋 배정 (3-5절). 사용된 방마다 에셋 인덱스(FPMGRoom::VariantIndex)와 회전(FPMGRoom::Rotation)을 정한다.
	 *
	 * (방 타입, 방 크기) 그룹마다 따로 배정한다. 그룹 순서는 Entrance, Exit, Standard, 커스텀 타입(AssetLists.CustomTypes 순서),
	 * 각 타입 안에서 1칸 → 2칸 → 4칸이고, 그룹 안의 방은 방 ID 오름차순이다.
	 * - 리스트(그 타입의 그 크기 리스트)가 비어 있으면: 그 방들은 VariantIndex = INDEX_NONE (스폰 생략) + OutWarnings에 경고
	 * - 아니면 셔플 백: 리스트 인덱스를 섞어 앞에서부터 꺼내고, 다 쓰면 다시 섞는다.
	 *   리스트가 방 수 이상이면 같은 에셋이 두 번 나오지 않는다. 모자라면 OutInfos에 "같은 에셋이 반복됨"
	 * - 고른 에셋의 bAllowRotation이 켜져 있으면 회전: 1칸/4칸 방은 0~3, 2칸 방은 0 또는 2 (90°를 더 돌리면 가로·세로가 바뀜)
	 *
	 * 미사용 방은 바꾸지 않는다 (VariantIndex = INDEX_NONE). 실패하지 않는다.
	 * 메시지는 로그를 남기지 않고 OutWarnings/OutInfos에 더하기만 한다 (로그는 부르는 쪽이 남긴다. 테스트가 메시지를 배열로 바로 검사할 수 있게).
	 * Layout에는 방 타입 배정(Phase 7)까지 끝나 있어야 한다.
	 */
	PROCMAPGEN_API void AssignRoomAssets(FPMGLayout& Layout, const FPMGAssetLists& AssetLists, FRandomStream& Stream,
		TArray<FString>& OutWarnings, TArray<FString>& OutInfos);

	/**
	 * Phase 9 ② 커넥터 목록 (3-4절). 스폰되는 방(VariantIndex가 있는 사용된 방)이 하나라도 접한 셀 경계마다 커넥터 하나를 Layout.Connectors에 만든다.
	 * 셀 경계 = 상하좌우로 맞닿은 두 셀의 방이 다르거나, 한쪽이 맵 밖인 곳.
	 * - 두 방이 모두 스폰되고, 그 경계가 문(Layout.Doors)의 자리면 → 문 (CellA/CellB는 문의 방향 그대로)
	 * - 그 밖 (연결됐지만 문 자리가 아닌 경계, 연결되지 않은 두 방, 한쪽만 스폰, 맵 테두리) → 벽
	 * 문이 있어도 한쪽 방이 스폰되지 않으면 벽이다.
	 *
	 * 순서: 셀을 위쪽 줄부터, 왼쪽부터 보고, 셀마다 +X, +Y, -X, -Y 방향의 경계 (안쪽 경계는 +X, +Y 쪽에서만 세어 한 번씩, 맵 테두리는 네 방향 모두).
	 * 에셋은 정하지 않는다 (PMG::AssignConnectorAssets). 난수를 쓰지 않는다. Layout에는 방 에셋 배정(PMG::AssignRoomAssets)이 끝나 있어야 한다.
	 */
	PROCMAPGEN_API void BuildConnectors(FPMGLayout& Layout);

	/**
	 * Phase 9 ③ 문/벽 에셋 배정 (3-4절 "문 에셋 고르기", 3-5절).
	 * 1. 문 커넥터마다 쓸 문 리스트를 고른다 (FPMGConnector::DoorListRoomType): 문의 RoomB(들어가는 쪽) 방 타입의 Doors → RoomA 방 타입의 Doors → 전역 Doors.
	 *    처음으로 비어 있지 않은 리스트를 쓴다
	 * 2. 리스트마다 그룹을 나눠 방 에셋과 같은 셔플 백으로 배정한다. 그룹 순서: 방 타입의 Doors(Entrance, Exit, Standard, 커스텀 타입 순서) → 전역 Doors → Walls.
	 *    그룹 안에서는 커넥터 순서. 리스트가 비어 있으면 INDEX_NONE (전역 Doors/Walls가 빈 경우. 경고는 UPMGSettings::Validate()가 이미 남김)
	 * 회전은 정하지 않는다 (위치와 방향은 경계가 정함). 그래서 문/벽 에셋의 bAllowRotation은 보지 않는다.
	 * 실패하지 않는다. "같은 에셋이 반복됨" 메시지는 OutInfos에 더한다. Layout에는 커넥터 목록(PMG::BuildConnectors)이 있어야 한다.
	 */
	PROCMAPGEN_API void AssignConnectorAssets(FPMGLayout& Layout, const FPMGAssetLists& AssetLists, FRandomStream& Stream, TArray<FString>& OutInfos);

	/**
	 * Phase 10 ④ 채움 방 에셋 배정 (미사용 방 정책 SpawnFiller). 미사용 방마다 AssetLists.FillerRooms의 인덱스(FPMGRoom::VariantIndex)와 회전을 정한다.
	 * 방 에셋과 같은 셔플 백(미사용 방 전체가 한 그룹, 방 ID 오름차순)과 회전 규칙을 쓴다.
	 * 리스트가 비어 있으면(정책이 DontSpawn이면 항상 비어 있음) 아무것도 바꾸지 않고 난수도 쓰지 않는다.
	 *
	 * 커넥터 목록은 바뀌지 않는다: 커넥터는 채움 방을 "스폰되지 않는 쪽"으로 본다 (3-4절 표, 스폰되는 방 = 에셋이 있는 사용된 방).
	 * 다른 에셋 배정의 결과가 바뀌지 않도록 에셋 배정의 맨 마지막(PMG::AssignConnectorAssets 다음)에 부른다.
	 * 실패하지 않는다. "같은 에셋이 반복됨" 메시지는 OutInfos에 더한다.
	 */
	PROCMAPGEN_API void AssignFillerAssets(FPMGLayout& Layout, const FPMGAssetLists& AssetLists, FRandomStream& Stream, TArray<FString>& OutInfos);
}
