#include "Data/PMGSettings.h"
#include "Core/PMGLayout.h"
#include "Core/PMGEntranceExit.h"
#include "PMGLog.h"

namespace
{
	/**
	 * 에셋 리스트의 모든 항목에 회전을 쓸 수 있는지(bCanRotate) 채운다.
	 * false면 Allow Rotation 체크박스가 숨겨지므로 bAllowRotation도 끈다
	 */
	void SetCanRotate(FPMGAssetSet& AssetSet, bool bCanRotate)
	{
		for (FPMGAssetVariant& Variant : AssetSet.Variants)
		{
			Variant.bCanRotate = bCanRotate;
			if (!bCanRotate)
			{
				Variant.bAllowRotation = false;
			}
		}
	}

	/** 한 방 타입의 리스트에 bCanRotate를 채운다: 방 리스트는 true, 문 리스트는 false */
	void SetRoomTypeCanRotate(FPMGRoomTypeAssets& Assets)
	{
		SetCanRotate(Assets.SingleRooms, true);
		SetCanRotate(Assets.DualRooms, true);
		SetCanRotate(Assets.QuadRooms, true);
		SetCanRotate(Assets.Doors, false);
	}

	/**
	 * 리스트의 항목이 Level과 ActorClass 중 하나만 지정했는지 검사한다 (종류마다 한 줄, 항목 번호를 나열).
	 * - 둘 다 지정: 에러 (즉시 실패). 레벨인지 액터인지 의도를 알 수 없는 명백한 잘못이라 고칠 때까지 생성하지 않는다
	 * - 둘 다 비어 있음: 경고. 리스트가 빈 것과 같이 그 항목이 배정된 곳은 스폰 생략. 에셋을 고르기 전에 항목만 추가해 구조를 미리 보는 경우도 있다
	 */
	void CheckVariants(const FPMGAssetSet& AssetSet, const FString& ListName, FPMGValidationResult& OutResult)
	{
		TArray<FString> BothSetIndices;
		TArray<FString> EmptyIndices;
		for (int32 Index = 0; Index < AssetSet.Variants.Num(); ++Index)
		{
			const FPMGAssetVariant& Variant = AssetSet.Variants[Index];
			const bool bHasLevel = !Variant.Level.IsNull();
			const bool bHasActorClass = !Variant.ActorClass.IsNull();
			if (bHasLevel && bHasActorClass)
			{
				BothSetIndices.Add(FString::FromInt(Index));
			}
			else if (!bHasLevel && !bHasActorClass)
			{
				EmptyIndices.Add(FString::FromInt(Index));
			}
		}
		if (!BothSetIndices.IsEmpty())
		{
			OutResult.Errors.Add(FString::Printf(TEXT("%s 항목 [%s]: Level과 ActorClass가 모두 지정됨 (둘 중 하나만 지정)"),
				*ListName, *FString::Join(BothSetIndices, TEXT(", "))));
		}
		if (!EmptyIndices.IsEmpty())
		{
			OutResult.Warnings.Add(FString::Printf(TEXT("%s 항목 [%s]: Level과 ActorClass가 모두 비어 있음 → 이 항목이 배정된 곳은 스폰 생략"),
				*ListName, *FString::Join(EmptyIndices, TEXT(", "))));
		}
	}

	/** 한 방 타입의 방 리스트(1칸/2칸/4칸)와 문 리스트에 CheckVariants. TypeName은 메시지에 표시할 이름 (예: "StandardRooms") */
	void CheckRoomTypeVariants(const FPMGRoomTypeAssets& Assets, const FString& TypeName, FPMGValidationResult& OutResult)
	{
		CheckVariants(Assets.SingleRooms, TypeName + TEXT(".SingleRooms"), OutResult);
		CheckVariants(Assets.DualRooms, TypeName + TEXT(".DualRooms"), OutResult);
		CheckVariants(Assets.QuadRooms, TypeName + TEXT(".QuadRooms"), OutResult);
		CheckVariants(Assets.Doors, TypeName + TEXT(".Doors"), OutResult);
	}

	FString RoomSizesToString(const FPMGRoomSizeSet& Sizes)
	{
		TArray<FString> Parts;
		if (Sizes.bSingle) { Parts.Add(TEXT("1칸")); }
		if (Sizes.bDual) { Parts.Add(TEXT("2칸")); }
		if (Sizes.bQuad) { Parts.Add(TEXT("4칸")); }
		return FString::Join(Parts, TEXT("/"));
	}

	/**
	 * 루트에 필요한 방 수 추정 (1-2절 예산 경고, 휴리스틱). 한 시도에서 입구/출구 크기가 Sizes일 때:
	 * - 추가방 예약 = 예약할 방 수 (입구/출구가 쓰고 남은 큰 방 수까지). 루트 옆에 방 하나씩 붙는다
	 * - 루트에서 만들 큰 방 수 = 남은 큰 방 수 - 예약
	 * - 입구/출구 거리 목표가 D이면 두 루트는 각각 방 D + 1개쯤이고 입구/출구를 같이 쓰므로 합집합은 약 2D.
	 *   루트 위 큰 방을 하나 만들 때마다 루트 칸 2개 이상이 방 하나가 되므로 방이 1개 이상 준다
	 */
	int32 EstimateRouteRoomCount(int32 Rows, int32 Cols, int32 QuadRoomCount, int32 DualRoomCount, int32 MinEntranceExitDistance,
		const FPMGEntranceExitSizes& Sizes, int32 ReservedRoomChoiceCount)
	{
		const int32 AvailableBigRoomCount = QuadRoomCount + DualRoomCount - Sizes.CountRoomsOfSize(EPMGRoomSize::Quad) - Sizes.CountRoomsOfSize(EPMGRoomSize::Dual);
		const int32 ReservedRoomCount = FMath::Min(ReservedRoomChoiceCount, AvailableBigRoomCount);
		const int32 RouteBigRoomCount = AvailableBigRoomCount - ReservedRoomCount;
		const int32 TargetDistance = PMG::GetTargetEntranceExitDistance(Rows, Cols, Sizes, MinEntranceExitDistance, RouteBigRoomCount);

		const int32 RouteRoomCount = FMath::Max(2, 2 * TargetDistance - RouteBigRoomCount);
		return RouteRoomCount + ReservedRoomCount;
	}
}

FPMGValidationResult UPMGSettings::Validate() const
{
	FPMGValidationResult Result;

	// ---------- 즉시 실패: 범위 ----------
	if (Rows < 1 || Rows > 100 || Cols < 1 || Cols > 100)
	{
		Result.Errors.Add(FString::Printf(TEXT("Rows/Cols는 1~100 범위여야 함 (Rows=%d, Cols=%d)"), Rows, Cols));
	}
	if (QuadRoomCount < 0 || DualRoomCount < 0)
	{
		Result.Errors.Add(FString::Printf(TEXT("QuadRoomCount/DualRoomCount는 0 이상이어야 함 (QuadRoomCount=%d, DualRoomCount=%d)"), QuadRoomCount, DualRoomCount));
	}
	if (!Result.Errors.IsEmpty())
	{
		return Result;
	}

	// ---------- 용량: 큰 방을 맵에 서로 겹치지 않게 놓을 수 있는지 ----------
	Result.MaxQuadRoomCount = PMG::GetMaxQuadRoomCount(Rows, Cols);
	Result.MaxDualRoomCount = FMath::Max(0, PMG::GetMaxDualRoomCount(Rows, Cols, QuadRoomCount));

	if (QuadRoomCount > Result.MaxQuadRoomCount)
	{
		Result.Errors.Add(FString::Printf(TEXT("QuadRoomCount(%d)가 최대값 floor(Rows/2)*floor(Cols/2) = %d를 초과"), QuadRoomCount, Result.MaxQuadRoomCount));
	}
	else if (DualRoomCount > Result.MaxDualRoomCount)
	{
		Result.Errors.Add(FString::Printf(TEXT("DualRoomCount(%d)가 최대값 floor((Rows*Cols - 4*QuadRoomCount)/2) = %d를 초과"), DualRoomCount, Result.MaxDualRoomCount));
	}
	if (!Result.Errors.IsEmpty())
	{
		return Result;
	}

	const int32 BigRoomCount = QuadRoomCount + DualRoomCount;
	Result.SingleRoomCount = Rows * Cols - 4 * QuadRoomCount - 2 * DualRoomCount;
	Result.AllRoomCount = BigRoomCount + Result.SingleRoomCount;

	// ---------- TotalRoomCount ----------
	Result.TotalRoomCount = TotalRoomCount;
	if (Result.TotalRoomCount > Result.AllRoomCount)
	{
		Result.Warnings.Add(FString::Printf(TEXT("TotalRoomCount(%d)가 전체 방 수(%d)보다 큼 → %d로 보정"), TotalRoomCount, Result.AllRoomCount, Result.AllRoomCount));
		Result.TotalRoomCount = Result.AllRoomCount;
	}
	const int32 MinTotalRoomCount = FMath::Max(2, BigRoomCount);
	if (Result.TotalRoomCount < MinTotalRoomCount)
	{
		Result.Errors.Add(FString::Printf(TEXT("TotalRoomCount(%d)가 최소값 max(2, QuadRoomCount+DualRoomCount) = %d보다 작음"), Result.TotalRoomCount, MinTotalRoomCount));
	}
	Result.UsedSingleRoomCount = Result.TotalRoomCount - BigRoomCount;

	// ---------- MinEntranceExitDistance ----------
	const int32 MaxManhattanDistance = (Rows - 1) + (Cols - 1);
	const bool bDistanceInsideMap = MinEntranceExitDistance <= MaxManhattanDistance;
	if (!bDistanceInsideMap)
	{
		Result.Errors.Add(FString::Printf(TEXT("MinEntranceExitDistance(%d)가 맵의 최대 맨해튼 거리(%d)를 초과"), MinEntranceExitDistance, MaxManhattanDistance));
	}

	// ---------- 입구/출구 크기 ----------
	Result.EntranceRoomSizes = EntranceRooms.GetRegisteredRoomSizes();
	if (Result.EntranceRoomSizes.IsEmpty())
	{
		Result.Warnings.Add(TEXT("EntranceRooms 리스트가 모두 비어 있음 → 입구 후보를 1칸으로 보고 진행 (입구 방은 스폰 생략)"));
		Result.EntranceRoomSizes.bSingle = true;
	}
	Result.ExitRoomSizes = ExitRooms.GetRegisteredRoomSizes();
	if (Result.ExitRoomSizes.IsEmpty())
	{
		Result.Warnings.Add(TEXT("ExitRooms 리스트가 모두 비어 있음 → 출구 후보를 1칸으로 보고 진행 (출구 방은 스폰 생략)"));
		Result.ExitRoomSizes.bSingle = true;
	}

	// 입구 크기 x 출구 크기 중 방 수와 거리 조건을 만족할 수 있는 조합. 시도마다 이 중 하나를 고른다
	// (맵 최대 거리를 넘는 경우는 위에서 이미 에러이므로 조합을 찾지 않는다)
	if (bDistanceInsideMap)
	{
		Result.EntranceExitSizeCombos = PMG::MakeEntranceExitSizeCombos(Rows, Cols, QuadRoomCount, DualRoomCount,
			Result.EntranceRoomSizes, Result.ExitRoomSizes, MinEntranceExitDistance);
		if (Result.EntranceExitSizeCombos.IsEmpty())
		{
			Result.Errors.Add(FString::Printf(TEXT("입구/출구 크기 조합이 없음: 입구 %s x 출구 %s 중 방 수(4칸 %d, 2칸 %d, 1칸 %d)가 모자라거나 맵 안에서 MinEntranceExitDistance(%d)만큼 떨어질 수 없음"),
				*RoomSizesToString(Result.EntranceRoomSizes), *RoomSizesToString(Result.ExitRoomSizes),
				QuadRoomCount, DualRoomCount, Result.SingleRoomCount, MinEntranceExitDistance));
		}
	}

	// ---------- StandardRooms ----------
	if (StandardRooms.SingleRooms.IsEmpty())
	{
		Result.Errors.Add(TEXT("StandardRooms의 1칸 리스트가 비어 있음"));
	}

	// ---------- 커스텀 타입 ----------
	TSet<FName> SeenNames;
	for (int32 Index = 0; Index < CustomTypes.Num(); ++Index)
	{
		const FName TypeName = CustomTypes[Index].TypeName;
		if (TypeName.IsNone())
		{
			Result.Warnings.Add(FString::Printf(TEXT("CustomTypes[%d]: 타입 이름이 비어 있음 → 무시"), Index));
			continue;
		}
		if (TypeName == PMG::RoomTypeEntrance() || TypeName == PMG::RoomTypeExit() || TypeName == PMG::RoomTypeStandard())
		{
			Result.Warnings.Add(FString::Printf(TEXT("CustomTypes[%d]: '%s'는 예약어 → 무시"), Index, *TypeName.ToString()));
			continue;
		}
		if (SeenNames.Contains(TypeName))
		{
			Result.Warnings.Add(FString::Printf(TEXT("CustomTypes[%d]: '%s' 이름 중복 → 무시"), Index, *TypeName.ToString()));
			continue;
		}
		SeenNames.Add(TypeName);
		Result.ValidCustomTypeIndices.Add(Index);
	}

	// ---------- 커스텀 타입 Count: 어떤 시도로도 채울 수 없는 수 (즉시 실패) ----------
	// 커스텀 타입은 사용된 방 중 입구/출구가 아닌 방에만, 그리고 그 타입에 방 에셋이 등록된 크기의 방에만 배정된다.
	// Count가 그 상한보다 크면 모든 시도가 방 타입 배정에서 실패하므로 시도하지 않는다 (상한만 보는 검사라, 통과해도 시도마다 모자랄 수는 있다)
	int32 CustomRoomCountTotal = 0;
	for (const int32 Index : Result.ValidCustomTypeIndices)
	{
		const FPMGCustomRoomType& CustomType = CustomTypes[Index];
		if (CustomType.Count <= 0)
		{
			continue;
		}
		CustomRoomCountTotal += CustomType.Count;

		const FPMGRoomSizeSet TypeRoomSizes = CustomType.Assets.GetRegisteredRoomSizes();
		int32 PossibleRoomCount = 0;   // 사용되는 방 중 이 타입에 에셋이 있는 크기의 방 수
		if (TypeRoomSizes.bSingle) { PossibleRoomCount += FMath::Max(0, Result.UsedSingleRoomCount); }
		if (TypeRoomSizes.bDual) { PossibleRoomCount += DualRoomCount; }
		if (TypeRoomSizes.bQuad) { PossibleRoomCount += QuadRoomCount; }
		if (CustomType.Count > PossibleRoomCount)
		{
			const FString SizesText = TypeRoomSizes.IsEmpty() ? TEXT("방 에셋 없음") : TEXT("방 에셋이 있는 크기 ") + RoomSizesToString(TypeRoomSizes);
			Result.Errors.Add(FString::Printf(TEXT("CustomTypes[%d] '%s': Count(%d)가 이 타입이 될 수 있는 사용되는 방 수(%d)보다 큼 (%s)"),
				Index, *CustomType.TypeName.ToString(), CustomType.Count, PossibleRoomCount, *SizesText));
		}
	}
	const int32 MaxCustomRoomCount = FMath::Max(0, Result.TotalRoomCount - 2);   // 입구방과 출구방은 커스텀 타입이 되지 않는다
	if (CustomRoomCountTotal > MaxCustomRoomCount)
	{
		Result.Errors.Add(FString::Printf(TEXT("커스텀 타입 Count의 합(%d)이 입구/출구를 뺀 사용되는 방 수 TotalRoomCount - 2 = %d보다 큼"),
			CustomRoomCountTotal, MaxCustomRoomCount));
	}

	// ---------- 추가방용 큰 방 예약 (2-9절) ----------
	// 추가방에만 배정되고 1칸 에셋이 없는 타입은 추가방 중에 큰 방이 있어야 배정될 수 있다. 큰 방 수보다 많이 예약할 수는 없다
	for (const int32 Index : Result.ValidCustomTypeIndices)
	{
		const FPMGCustomRoomType& CustomType = CustomTypes[Index];
		if (!CustomType.NeedsReservedBigRoom())
		{
			continue;
		}
		const FPMGRoomSizeSet BigRoomSizes = CustomType.Assets.GetRegisteredRoomSizes();   // 1칸 에셋이 없으므로 큰 방 크기(bDual/bQuad)만 들어 있다
		for (int32 Count = 0; Count < CustomType.Count && Result.ReservedRoomSizeChoices.Num() < BigRoomCount; ++Count)
		{
			Result.ReservedRoomSizeChoices.Add(BigRoomSizes);
		}
	}

	// ---------- 문/벽 ----------
	if (Doors.IsEmpty())
	{
		Result.Warnings.Add(TEXT("Doors가 비어 있음 → 양쪽 방 타입에도 Doors가 없는 문은 문 없이 개구부만 열림"));
	}
	if (Walls.IsEmpty())
	{
		Result.Warnings.Add(TEXT("Walls가 비어 있음 → 연결되지 않은 개구부가 열린 채로 남음"));
	}

	// ---------- 채움 방 ----------
	if (UnusedRoomPolicy == EPMGUnusedRoomPolicy::SpawnFiller && FillerRooms.IsEmpty())
	{
		Result.Warnings.Add(TEXT("UnusedRoomPolicy가 SpawnFiller인데 FillerRooms가 비어 있음 → 미사용 방에 아무것도 스폰하지 않음"));
	}

	// ---------- 에셋 항목: Level과 ActorClass 중 하나만 (Roadmap 3-1절). 둘 다 지정은 에러, 둘 다 비어 있음은 경고 ----------
	// 쓰이는 리스트만 본다: 무시되는 커스텀 타입은 빼고, 채움 방은 SpawnFiller일 때만
	CheckRoomTypeVariants(EntranceRooms, TEXT("EntranceRooms"), Result);
	CheckRoomTypeVariants(ExitRooms, TEXT("ExitRooms"), Result);
	CheckRoomTypeVariants(StandardRooms, TEXT("StandardRooms"), Result);
	for (const int32 Index : Result.ValidCustomTypeIndices)
	{
		CheckRoomTypeVariants(CustomTypes[Index].Assets, FString::Printf(TEXT("CustomTypes[%d] '%s'"), Index, *CustomTypes[Index].TypeName.ToString()), Result);
	}
	CheckVariants(Doors, TEXT("Doors"), Result);
	CheckVariants(Walls, TEXT("Walls"), Result);
	if (UnusedRoomPolicy == EPMGUnusedRoomPolicy::SpawnFiller)
	{
		CheckVariants(FillerRooms, TEXT("FillerRooms"), Result);
	}

	// ---------- 비용 / 시도 횟수 ----------
	// 다른 방의 비용은 1이므로, 페널티가 1 이하이면 루트 2가 루트 1을 피할 이유가 없다
	Result.RouteCostPenalty = RouteCostPenalty;
	if (RouteCostPenalty <= 1)
	{
		Result.RouteCostPenalty = 2;
		Result.Warnings.Add(FString::Printf(TEXT("RouteCostPenalty(%d) <= 1 (다른 방의 비용) → 2로 보정"), RouteCostPenalty));
	}

	Result.MaxAttempts = MaxAttempts;
	if (MaxAttempts <= 0)
	{
		Result.MaxAttempts = PMG::DefaultMaxAttempts;
		Result.Warnings.Add(FString::Printf(TEXT("MaxAttempts(%d) <= 0 → 기본값 %d 사용"), MaxAttempts, PMG::DefaultMaxAttempts));
	}

	// ---------- 예산 부족 추정 (경고 전용) ----------
	// 입구/출구 크기 조합마다 추정해 가장 큰 값을 쓴다 (어느 조합이 뽑혀도 부족하지 않은지)
	Result.RecommendedTotalRoomCount = MinTotalRoomCount;
	for (const FPMGEntranceExitSizes& Sizes : Result.EntranceExitSizeCombos)
	{
		const int32 Estimated = EstimateRouteRoomCount(Rows, Cols, QuadRoomCount, DualRoomCount, MinEntranceExitDistance, Sizes, Result.ReservedRoomSizeChoices.Num());
		Result.EstimatedRouteRoomCount = FMath::Max(Result.EstimatedRouteRoomCount, Estimated);
	}
	if (Result.EstimatedRouteRoomCount > 0)
	{
		Result.RecommendedTotalRoomCount = FMath::Clamp(Result.EstimatedRouteRoomCount, MinTotalRoomCount, Result.AllRoomCount);
		if (Result.TotalRoomCount >= MinTotalRoomCount && Result.TotalRoomCount < Result.EstimatedRouteRoomCount)
		{
			Result.Warnings.Add(FString::Printf(TEXT("TotalRoomCount(%d)가 루트에 필요한 방 수 추정(%d)보다 작아 실패할 수 있음 (권장 TotalRoomCount >= %d)"),
				Result.TotalRoomCount, Result.EstimatedRouteRoomCount, Result.RecommendedTotalRoomCount));
		}
	}

	Result.bValid = Result.Errors.IsEmpty();
	return Result;
}

void UPMGSettings::RefreshInfo(bool bLogMessages)
{
	const FPMGValidationResult Result = Validate();

	SingleRoomCount = Result.SingleRoomCount;
	UsedSingleRoomCount = Result.UsedSingleRoomCount;
	AllRoomCount = Result.AllRoomCount;
	MaxQuadRoomCount = Result.MaxQuadRoomCount;
	MaxDualRoomCount = Result.MaxDualRoomCount;
	EstimatedRouteRoomCount = Result.EstimatedRouteRoomCount;
	RecommendedTotalRoomCount = Result.RecommendedTotalRoomCount;

	ValidationMessages.Reset();
	for (const FString& Error : Result.Errors)
	{
		ValidationMessages.Add(TEXT("[Error] ") + Error);
	}
	for (const FString& Warning : Result.Warnings)
	{
		ValidationMessages.Add(TEXT("[Warning] ") + Warning);
	}

	if (!Result.bValid)
	{
		ValidationStatus = FString::Printf(TEXT("Fail (에러 %d)"), Result.Errors.Num());
	}
	else if (!Result.Warnings.IsEmpty())
	{
		ValidationStatus = FString::Printf(TEXT("OK (경고 %d)"), Result.Warnings.Num());
	}
	else
	{
		ValidationStatus = TEXT("OK");
	}

	if (bLogMessages)
	{
		for (const FString& Error : Result.Errors)
		{
			PMG::LogError(FString::Printf(TEXT("%s: %s"), *GetName(), *Error));
		}
		for (const FString& Warning : Result.Warnings)
		{
			UE_LOG(LogProcMapGen, Warning, TEXT("%s: %s"), *GetName(), *Warning);
		}
	}
}

void UPMGSettings::PostInitProperties()
{
	Super::PostInitProperties();
	RefreshInfo(false);
}

void UPMGSettings::PostLoad()
{
	Super::PostLoad();
	RefreshCanRotate();
	RefreshInfo(false);
}

#if WITH_EDITOR
void UPMGSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// 문/벽 리스트에 항목을 추가하면 새 항목은 bCanRotate = true로 만들어지므로, 바뀔 때마다 다시 채운다
	RefreshCanRotate();
	RefreshInfo(true);
}
#endif

void UPMGSettings::RefreshCanRotate()
{
	SetRoomTypeCanRotate(EntranceRooms);
	SetRoomTypeCanRotate(ExitRooms);
	SetRoomTypeCanRotate(StandardRooms);
	for (FPMGCustomRoomType& CustomType : CustomTypes)
	{
		SetRoomTypeCanRotate(CustomType.Assets);
	}
	SetCanRotate(FillerRooms, true);
	SetCanRotate(Doors, false);
	SetCanRotate(Walls, false);
}
