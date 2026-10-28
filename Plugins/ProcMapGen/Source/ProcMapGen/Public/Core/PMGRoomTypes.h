#pragma once

#include "CoreMinimal.h"
#include "Core/PMGLayout.h"

namespace PMG
{
	/**
	 * Phase 7: 사용된 방에 커스텀 방 타입을 배정한다 (2-9절 "커스텀 타입 배정").
	 * 입구방(Entrance)과 출구방(Exit)은 Phase 3에서 정해져 있고 나머지 방은 기본값 Standard다.
	 * 이 함수는 사용된 Standard 방 중 일부를 커스텀 타입으로 바꾼다 (FPMGRoom::RoomType = 타입 이름).
	 *
	 * 타입마다 (CustomTypes 중 ValidCustomTypeIndices에 든 것만, Count가 1 이상인 것만):
	 * 1. 대상 = 사용된(bConnected) Standard 방 중 그 타입에 방 에셋이 등록된 크기의 방. bExtraRoom이면 추가방(bExtra)만.
	 *    bAvoidDoorToEntrance면 입구방과, bAvoidDoorToExit면 출구방과 문으로 바로 이어진 방은 뺀다
	 * 2. 대상 중 StandardRooms에 그 크기 에셋이 없는 방을 먼저 (Standard로 남으면 스폰되지 않는 방),
	 *    남은 수만큼 나머지 대상에서, 각각 무작위로 Count개를 고른다
	 * 3. 대상이 Count보다 적으면 시도 실패: OutError에 사유를 쓰고 false
	 *
	 * 처리 순서 (제약이 큰 타입 우선. 먼저 처리한 타입이 방을 먼저 가져간다):
	 * 1. 큰 방 예약 대상 타입(FPMGCustomRoomType::NeedsReservedBigRoom)을 먼저, 그다음 나머지 타입.
	 *    예약한 추가방 큰 방을 같은 크기 에셋을 가진 다른 타입이 먼저 가져가지 않게 한다
	 * 2. 같은 묶음 안에서는 방 에셋이 등록된 크기 수가 적은 타입부터
	 * 3. 크기 수도 같으면 추가방 전용(bExtraRoom) 타입부터 (대상이 추가방뿐이라 크기가 같은 일반 타입보다 고를 방이 적다)
	 * 4. 그것도 같으면 문 제외 옵션(bAvoidDoorToEntrance, bAvoidDoorToExit)을 더 많이 켠 타입부터 (같은 이유)
	 * 5. 그래도 같으면 정의 순서 (CustomTypes 인덱스)
	 *
	 * 실패하면 그 타입은 배정하지 않고 바로 돌아온다. 앞에서 처리한 타입의 배정은 Layout에 남는다 (시도 실패라 생성기는 이 Layout을 버린다).
	 * Layout에는 분기 채우기(Phase 6 PMG::FillBranches)까지 끝나 있어야 한다.
	 */
	PROCMAPGEN_API bool AssignCustomRoomTypes(FPMGLayout& Layout, const TArray<FPMGCustomRoomType>& CustomTypes, const TArray<int32>& ValidCustomTypeIndices,
		const FPMGRoomTypeAssets& StandardRooms, FRandomStream& Stream, FString& OutError);
}
