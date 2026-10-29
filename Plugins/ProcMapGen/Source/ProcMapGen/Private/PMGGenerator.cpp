#include "PMGGenerator.h"
#include "Core/PMGLayout.h"
#include "Core/PMGNeighbors.h"
#include "Core/PMGEntranceExit.h"
#include "Core/PMGRoute.h"
#include "Core/PMGBigRooms.h"
#include "Core/PMGDoors.h"
#include "Core/PMGBranches.h"
#include "Core/PMGRoomTypes.h"
#include "Core/PMGAssets.h"
#include "Data/PMGSettings.h"
#include "PMGLog.h"

namespace
{
	/** 시도 실패가 일어날 수 있는 단계 (1-2절 ③). 실패 사유별 횟수를 이 단계 단위로 센다 */
	enum class EGenerationStep : uint8
	{
		EntranceExit,
		Routes,
		RouteBigRooms,
		BigRoomsBesideRoutes,
		Branches,
		RoomTypes,
		FinalCheck
	};

	/** EGenerationStep 항목 수 (단계별 실패 횟수 배열의 크기) */
	constexpr int32 GenerationStepCount = 7;

	const TCHAR* GetGenerationStepName(EGenerationStep Step)
	{
		switch (Step)
		{
		case EGenerationStep::EntranceExit:
			return TEXT("입구/출구 지정");
		case EGenerationStep::Routes:
			return TEXT("루트 찾기");
		case EGenerationStep::RouteBigRooms:
			return TEXT("루트 위 큰 방");
		case EGenerationStep::BigRoomsBesideRoutes:
			return TEXT("루트 옆 큰 방");
		case EGenerationStep::Branches:
			return TEXT("분기 채우기");
		case EGenerationStep::RoomTypes:
			return TEXT("방 타입 배정");
		case EGenerationStep::FinalCheck:
			return TEXT("최종 검증");
		}
		return TEXT("?");
	}

	/**
	 * 시도 하나 (2-6절 1~8단계). 모든 단계가 Stream을 정해진 순서로 이어 쓴다.
	 * 성공하면 OutLayout에 완성된 맵을 담고 true.
	 * 실패하면 실패한 단계와 사유를 OutFailedStep, OutError에 쓰고 false (OutLayout은 거기까지 만든 상태).
	 * 입력은 설정 에셋 값과, 보정된 값·파생 값이 든 Validate() 결과다 (Validation.bValid가 true여야 한다).
	 */
	bool RunGenerationAttempt(const UPMGSettings& Settings, const FPMGValidationResult& Validation, FRandomStream& Stream,
		FPMGLayout& OutLayout, EGenerationStep& OutFailedStep, FString& OutError)
	{
		// 1~2. 모든 셀 1칸 방 → 이웃
		PMG::BuildLayout(Settings.Rows, Settings.Cols, OutLayout);
		PMG::BuildNeighbors(OutLayout);

		// 3. 입구/출구 크기 조합 → 추가방용 큰 방 예약 → 목표 거리 → 입구/출구 자리 (2-9절)
		const FPMGEntranceExitSizes Sizes = PMG::ChooseEntranceExitSizes(Validation.EntranceExitSizeCombos, Stream);
		const int32 AvailableQuadRoomCount = Settings.QuadRoomCount - Sizes.CountRoomsOfSize(EPMGRoomSize::Quad);
		const int32 AvailableDualRoomCount = Settings.DualRoomCount - Sizes.CountRoomsOfSize(EPMGRoomSize::Dual);
		int32 ReservedQuadRoomCount = 0;
		int32 ReservedDualRoomCount = 0;
		PMG::ChooseReservedBigRooms(Validation.ReservedRoomSizeChoices, AvailableQuadRoomCount, AvailableDualRoomCount, Stream,
			ReservedQuadRoomCount, ReservedDualRoomCount);
		const int32 RouteQuadRoomCount = AvailableQuadRoomCount - ReservedQuadRoomCount;   // 루트 위에서 만들 큰 방 수
		const int32 RouteDualRoomCount = AvailableDualRoomCount - ReservedDualRoomCount;
		const int32 TargetDistance = PMG::GetTargetEntranceExitDistance(Settings.Rows, Settings.Cols, Sizes, Settings.MinEntranceExitDistance,
			RouteQuadRoomCount + RouteDualRoomCount);
		if (!PMG::ChooseEntranceAndExit(OutLayout, Sizes, TargetDistance, Stream, OutError))
		{
			OutFailedStep = EGenerationStep::EntranceExit;
			return false;
		}

		// 4. 루트 1, 2 → 루트 위 큰 방 합치기 (+ 방 수 검사)
		if (!PMG::FindRoutes(OutLayout, Validation.RouteCostPenalty, Stream, OutError))
		{
			OutFailedStep = EGenerationStep::Routes;
			return false;
		}
		int32 LeftoverQuadRoomCount = 0;
		int32 LeftoverDualRoomCount = 0;
		if (!PMG::MergeRouteBigRooms(OutLayout, RouteQuadRoomCount, RouteDualRoomCount, ReservedQuadRoomCount + ReservedDualRoomCount,
			Validation.TotalRoomCount, Settings.SpreadCandidateCount, Stream, LeftoverQuadRoomCount, LeftoverDualRoomCount, OutError))
		{
			OutFailedStep = EGenerationStep::RouteBigRooms;
			return false;
		}

		// 5. 루트 연결 (실패하지 않는다)
		PMG::ConnectRoutes(OutLayout, Stream);

		// 6. 루트 옆 큰 방 (루트 위에 만들지 못한 큰 방 + 추가방 예약) → 분기 채우기
		if (!PMG::AttachBigRoomsBesideRoutes(OutLayout, LeftoverQuadRoomCount + ReservedQuadRoomCount, LeftoverDualRoomCount + ReservedDualRoomCount,
			Settings.SpreadCandidateCount, Stream, OutError))
		{
			OutFailedStep = EGenerationStep::BigRoomsBesideRoutes;
			return false;
		}
		if (!PMG::FillBranches(OutLayout, Validation.TotalRoomCount, Stream, OutError))
		{
			OutFailedStep = EGenerationStep::Branches;
			return false;
		}

		// 7. 방 타입 배정 (커스텀 타입의 대상이 Count보다 적으면 실패)
		if (!PMG::AssignCustomRoomTypes(OutLayout, Settings.CustomTypes, Validation.ValidCustomTypeIndices, Settings.StandardRooms, Stream, OutError))
		{
			OutFailedStep = EGenerationStep::RoomTypes;
			return false;
		}

		// 8. 최종 검증
		if (!PMG::VerifyGeneratedLayout(OutLayout, Settings.QuadRoomCount, Settings.DualRoomCount, Validation.TotalRoomCount, OutError))
		{
			OutFailedStep = EGenerationStep::FinalCheck;
			return false;
		}
		return true;
	}

	/** 완성된 Layout의 방, 문, 커넥터, 입구/출구, 루트를 결과에 옮긴다 (Core 구조체 → Blueprint에서 읽는 결과 구조체) */
	void CopyLayoutToResult(const FPMGLayout& Layout, FPMGResult& OutResult)
	{
		OutResult.Rooms.Reset(Layout.Rooms.Num());
		for (const FPMGRoom& Room : Layout.Rooms)
		{
			FPMGResultRoom& ResultRoom = OutResult.Rooms.AddDefaulted_GetRef();
			ResultRoom.Id = Room.Id;
			ResultRoom.Shape = Room.Shape;
			ResultRoom.Bounds = Room.Bounds;
			ResultRoom.RoomType = Room.RoomType;
			ResultRoom.Neighbors = Room.Neighbors;
			ResultRoom.bConnected = Room.bConnected;
			ResultRoom.bExtra = Room.bExtra;
			ResultRoom.VariantIndex = Room.VariantIndex;
			ResultRoom.Rotation = Room.Rotation;
		}

		OutResult.Doors.Reset(Layout.Doors.Num());
		for (const FPMGDoor& Door : Layout.Doors)
		{
			FPMGResultDoor& ResultDoor = OutResult.Doors.AddDefaulted_GetRef();
			ResultDoor.RoomA = Door.RoomA;
			ResultDoor.RoomB = Door.RoomB;
			ResultDoor.CellA = Door.CellA;
			ResultDoor.CellB = Door.CellB;
		}

		OutResult.Connectors.Reset(Layout.Connectors.Num());
		for (const FPMGConnector& Connector : Layout.Connectors)
		{
			FPMGResultConnector& ResultConnector = OutResult.Connectors.AddDefaulted_GetRef();
			ResultConnector.CellA = Connector.CellA;
			ResultConnector.CellB = Connector.CellB;
			ResultConnector.Type = Connector.Type;
			ResultConnector.DoorListRoomType = Connector.DoorListRoomType;
			ResultConnector.VariantIndex = Connector.VariantIndex;
		}

		OutResult.EntranceRoomId = Layout.EntranceRoomId;
		OutResult.ExitRoomId = Layout.ExitRoomId;
		OutResult.PrimaryRoute = Layout.PrimaryRoute;
		OutResult.SecondaryRoute = Layout.SecondaryRoute;
	}
}

int32 PMG::GetAttemptSeed(int32 Seed, int32 AttemptIndex)
{
	// 부호 없는 정수는 넘치면 0부터 다시 도는 것이 C++ 규칙으로 정해져 있다 (부호 있는 정수의 넘침은 정의되지 않음)
	const uint32 AttemptSeed = static_cast<uint32>(Seed) + static_cast<uint32>(AttemptIndex) * 0x9E3779B9u;
	return static_cast<int32>(AttemptSeed);
}

bool PMG::VerifyGeneratedLayout(const FPMGLayout& Layout, int32 QuadRoomCount, int32 DualRoomCount, int32 TotalRoomCount, FString& OutError)
{
	// 1) 방 구조
	FString LayoutError;
	if (!VerifyLayout(Layout, QuadRoomCount, DualRoomCount, LayoutError))
	{
		OutError = TEXT("방 구조: ") + LayoutError;
		return false;
	}
	if (!Layout.Rooms.IsValidIndex(Layout.EntranceRoomId) || !Layout.Rooms.IsValidIndex(Layout.ExitRoomId))
	{
		OutError = FString::Printf(TEXT("입구방(%d) 또는 출구방(%d)이 없음"), Layout.EntranceRoomId, Layout.ExitRoomId);
		return false;
	}

	// 2) 큰 방은 모두 사용, 사용된 방 수 = TotalRoomCount
	int32 ConnectedRoomCount = 0;
	for (const FPMGRoom& Room : Layout.Rooms)
	{
		if (Room.Shape != EPMGRoomShape::Single && !Room.bConnected)
		{
			OutError = FString::Printf(TEXT("큰 방 %d가 사용되지 않음"), Room.Id);
			return false;
		}
		if (Room.bConnected)
		{
			ConnectedRoomCount++;
		}
	}
	if (ConnectedRoomCount != TotalRoomCount)
	{
		OutError = FString::Printf(TEXT("사용된 방 %d개 != TotalRoomCount %d"), ConnectedRoomCount, TotalRoomCount);
		return false;
	}

	// 3) 연결성: 방마다 문으로 이어진 방 목록을 만들고, 입구방에서 너비 우선 탐색(BFS)
	TArray<TArray<int32>> DoorLinkedRooms;
	DoorLinkedRooms.SetNum(Layout.Rooms.Num());
	for (const FPMGDoor& Door : Layout.Doors)
	{
		DoorLinkedRooms[Door.RoomA].Add(Door.RoomB);
		DoorLinkedRooms[Door.RoomB].Add(Door.RoomA);
	}
	TArray<bool> bReached;
	bReached.Init(false, Layout.Rooms.Num());
	TArray<int32> Queue;
	Queue.Add(Layout.EntranceRoomId);
	bReached[Layout.EntranceRoomId] = true;
	for (int32 QueueIndex = 0; QueueIndex < Queue.Num(); ++QueueIndex)
	{
		for (const int32 LinkedRoomId : DoorLinkedRooms[Queue[QueueIndex]])
		{
			if (!bReached[LinkedRoomId])
			{
				bReached[LinkedRoomId] = true;
				Queue.Add(LinkedRoomId);
			}
		}
	}

	if (!bReached[Layout.ExitRoomId])
	{
		OutError = FString::Printf(TEXT("입구방 %d에서 문으로 출구방 %d에 도착하지 못함"), Layout.EntranceRoomId, Layout.ExitRoomId);
		return false;
	}
	for (const FPMGRoom& Room : Layout.Rooms)
	{
		if (Room.bConnected != bReached[Room.Id])
		{
			OutError = FString::Printf(TEXT("방 %d: 사용 여부(%s)와 입구방에서 문으로 도착하는지(%s)가 다름"), Room.Id,
				Room.bConnected ? TEXT("사용") : TEXT("미사용"), bReached[Room.Id] ? TEXT("도착") : TEXT("도착 못함"));
			return false;
		}
	}
	return true;
}

FPMGAssetLists PMG::MakeAssetLists(const UPMGSettings& Settings, const FPMGValidationResult& Validation)
{
	FPMGAssetLists AssetLists;
	AssetLists.EntranceRooms = Settings.EntranceRooms;
	AssetLists.ExitRooms = Settings.ExitRooms;
	AssetLists.StandardRooms = Settings.StandardRooms;
	for (const int32 CustomTypeIndex : Validation.ValidCustomTypeIndices)
	{
		AssetLists.CustomTypes.Add(Settings.CustomTypes[CustomTypeIndex]);
	}
	AssetLists.Doors = Settings.Doors;
	AssetLists.Walls = Settings.Walls;
	// 미사용 방 정책이 SpawnFiller일 때만 채움 방 리스트를 담는다. 비어 있으면 PMG::AssignFillerAssets가 아무것도 하지 않는다
	if (Settings.UnusedRoomPolicy == EPMGUnusedRoomPolicy::SpawnFiller)
	{
		AssetLists.FillerRooms = Settings.FillerRooms;
	}
	return AssetLists;
}

FPMGResult PMG::GenerateMap(const UPMGSettings& Settings, int32 Seed)
{
	FPMGResult Result;
	Result.Seed = Seed;   // 성공/실패와 상관없이 남긴다 (랜덤 시드로 생성했을 때 다시 보려면 필요)

	// 1. 설정 검증 (1-2절): 무시 규칙의 경고는 로그 + 결과, 즉시 실패면 시도하지 않는다
	const FPMGValidationResult Validation = Settings.Validate();
	for (const FString& Warning : Validation.Warnings)
	{
		AddWarning(Result.Warnings, Warning);
	}
	if (!Validation.bValid)
	{
		Result.FailReason = TEXT("설정 검증 실패: ") + Validation.GetFailReason();
		LogError(Result.FailReason);
		return Result;
	}

	// 2. 시도 루프 (2-6절). 실패하면 단계별로 세고 다음 시도로
	TArray<int32> FailCountOfStep;
	FailCountOfStep.Init(0, GenerationStepCount);
	EGenerationStep LastFailedStep = EGenerationStep::EntranceExit;
	FString LastError;

	for (int32 AttemptIndex = 0; AttemptIndex < Validation.MaxAttempts; ++AttemptIndex)
	{
		FRandomStream Stream(GetAttemptSeed(Seed, AttemptIndex));
		FPMGLayout Layout;
		EGenerationStep FailedStep = EGenerationStep::EntranceExit;
		FString Error;

		if (RunGenerationAttempt(Settings, Validation, Stream, Layout, FailedStep, Error))
		{
			// 3. 에셋 배정 (Phase 9, 10): 성공한 시도의 스트림을 이어 쓴다. 실패하지 않는다. 채움 방은 맨 마지막이라 다른 배정을 바꾸지 않는다
			const FPMGAssetLists AssetLists = MakeAssetLists(Settings, Validation);
			TArray<FString> AssetWarnings;
			TArray<FString> AssetInfos;
			AssignRoomAssets(Layout, AssetLists, Stream, AssetWarnings, AssetInfos);
			BuildConnectors(Layout);
			AssignConnectorAssets(Layout, AssetLists, Stream, AssetInfos);
			AssignFillerAssets(Layout, AssetLists, Stream, AssetInfos);

			// 4. 성공: 결과에 옮기고, 이 시도의 에셋 경고를 남긴다. "같은 에셋이 반복됨"은 정보라 Log 수준
			Result.bSuccess = true;
			Result.AttemptCount = AttemptIndex + 1;
			for (const FString& Warning : AssetWarnings)
			{
				AddWarning(Result.Warnings, Warning);
			}
			for (const FString& Info : AssetInfos)
			{
				UE_LOG(LogProcMapGen, Log, TEXT("%s"), *Info);
			}
			CopyLayoutToResult(Layout, Result);
			UE_LOG(LogProcMapGen, Log, TEXT("맵 생성 성공: Seed %d, 시도 %d회"), Seed, Result.AttemptCount);
			return Result;
		}

		FailCountOfStep[static_cast<int32>(FailedStep)]++;
		LastFailedStep = FailedStep;
		LastError = Error;
		// 실패한 시도는 다음 시도로 넘어가는 정상 흐름이라 기본으로는 보이지 않는 Verbose로 남긴다 (log LogProcMapGen Verbose로 보기)
		UE_LOG(LogProcMapGen, Verbose, TEXT("시도 %d 실패 (%s): %s"), AttemptIndex + 1, GetGenerationStepName(FailedStep), *Error);
	}

	// 5. 모든 시도 실패: 실패 사유는 숫자가 들어간 문장이라 그대로는 묶이지 않으므로, 단계별 횟수 + 마지막 사유를 남긴다
	TArray<FString> FailCountTexts;
	for (int32 StepIndex = 0; StepIndex < GenerationStepCount; ++StepIndex)
	{
		if (FailCountOfStep[StepIndex] > 0)
		{
			FailCountTexts.Add(FString::Printf(TEXT("%s %d회"), GetGenerationStepName(static_cast<EGenerationStep>(StepIndex)), FailCountOfStep[StepIndex]));
		}
	}
	Result.AttemptCount = Validation.MaxAttempts;
	Result.FailReason = FString::Printf(TEXT("시도 %d회 모두 실패 (단계별: %s). 마지막 실패 (%s): %s"), Validation.MaxAttempts,
		*FString::Join(FailCountTexts, TEXT(", ")), GetGenerationStepName(LastFailedStep), *LastError);
	LogError(Result.FailReason);
	return Result;
}
