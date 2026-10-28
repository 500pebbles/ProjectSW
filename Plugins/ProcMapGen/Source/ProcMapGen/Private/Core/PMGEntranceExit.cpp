#include "Core/PMGEntranceExit.h"
#include "Algo/RandomShuffle.h"

namespace
{
	/** 방 크기 순서 (조합 목록을 이 순서로 만든다) */
	const EPMGRoomSize AllRoomSizes[] = { EPMGRoomSize::Single, EPMGRoomSize::Dual, EPMGRoomSize::Quad };

	FString RoomSizeToString(EPMGRoomSize Size)
	{
		switch (Size)
		{
		case EPMGRoomSize::Dual:
			return TEXT("2칸");
		case EPMGRoomSize::Quad:
			return TEXT("4칸");
		default:
			return TEXT("1칸");
		}
	}

	// ============================================================
	// 자리 (방이 놓일 셀 범위)
	// ============================================================

	/** 두 자리 중심 사이의 맨해튼 거리 (GetRoomDistance와 같은 기준) */
	double GetRectDistance(const FIntRect& A, const FIntRect& B)
	{
		const FVector2D CenterA = PMG::GetRectCenter(A);
		const FVector2D CenterB = PMG::GetRectCenter(B);
		return FMath::Abs(CenterA.X - CenterB.X) + FMath::Abs(CenterA.Y - CenterB.Y);
	}

	/** Width x Height 모양이 맵 안에 놓일 수 있는 모든 자리 (왼쪽 위 셀이 위쪽 줄부터, 왼쪽부터) */
	void AddPlacesOfShape(int32 Rows, int32 Cols, int32 Width, int32 Height, TArray<FIntRect>& OutPlaces)
	{
		for (int32 Y = 0; Y + Height <= Rows; ++Y)
		{
			for (int32 X = 0; X + Width <= Cols; ++X)
			{
				OutPlaces.Add(FIntRect(X, Y, X + Width, Y + Height));
			}
		}
	}

	/** Size 크기의 방이 놓일 수 있는 모든 자리. 2칸은 가로 자리 다음에 세로 자리 */
	TArray<FIntRect> GetPlacesOfSize(int32 Rows, int32 Cols, EPMGRoomSize Size)
	{
		TArray<FIntRect> Places;
		switch (Size)
		{
		case EPMGRoomSize::Single:
			AddPlacesOfShape(Rows, Cols, 1, 1, Places);
			break;
		case EPMGRoomSize::Dual:
			AddPlacesOfShape(Rows, Cols, 2, 1, Places);   // 가로 2칸
			AddPlacesOfShape(Rows, Cols, 1, 2, Places);   // 세로 2칸
			break;
		case EPMGRoomSize::Quad:
			AddPlacesOfShape(Rows, Cols, 2, 2, Places);
			break;
		}
		return Places;
	}

	// ============================================================
	// 최대 거리
	// ============================================================

	/** 한 모양의 방 중심이 놓일 수 있는 범위 (가장 작은 / 가장 큰 X, Y) */
	struct FCenterRange
	{
		/** 맵 안에 놓일 수 있는 자리가 있는지 */
		bool bHasPlace = false;
		FVector2D Min = FVector2D::ZeroVector;
		FVector2D Max = FVector2D::ZeroVector;
	};

	/** Width x Height 모양의 중심 범위. 왼쪽 끝 자리의 중심이 Width/2, 오른쪽 끝 자리의 중심이 Cols - Width/2 (Y도 같다) */
	FCenterRange MakeCenterRange(int32 Rows, int32 Cols, int32 Width, int32 Height)
	{
		FCenterRange Range;
		if (Width > Cols || Height > Rows)
		{
			return Range;   // 맵에 들어가지 않는 모양
		}
		Range.bHasPlace = true;
		Range.Min = FVector2D(Width * 0.5, Height * 0.5);
		Range.Max = FVector2D(Cols - Width * 0.5, Rows - Height * 0.5);
		return Range;
	}

	/** Size 크기에 해당하는 모양들의 중심 범위 (2칸은 가로/세로 두 모양) */
	TArray<FCenterRange> GetCenterRanges(int32 Rows, int32 Cols, EPMGRoomSize Size)
	{
		switch (Size)
		{
		case EPMGRoomSize::Dual:
			return { MakeCenterRange(Rows, Cols, 2, 1), MakeCenterRange(Rows, Cols, 1, 2) };
		case EPMGRoomSize::Quad:
			return { MakeCenterRange(Rows, Cols, 2, 2) };
		default:
			return { MakeCenterRange(Rows, Cols, 1, 1) };
		}
	}

	/** 범위 A 안의 점과 범위 B 안의 점 사이에서 가능한 가장 먼 맨해튼 거리 */
	double GetFarthestDistance(const FCenterRange& A, const FCenterRange& B)
	{
		// 가로: A의 오른쪽 끝과 B의 왼쪽 끝, 또는 B의 오른쪽 끝과 A의 왼쪽 끝 중 더 먼 쪽. 세로도 같다
		const double FarthestX = FMath::Max(A.Max.X - B.Min.X, B.Max.X - A.Min.X);
		const double FarthestY = FMath::Max(A.Max.Y - B.Min.Y, B.Max.Y - A.Min.Y);
		return FarthestX + FarthestY;
	}
}

double PMG::GetRoomDistance(const FPMGRoom& RoomA, const FPMGRoom& RoomB)
{
	const FVector2D CenterA = RoomA.GetCenter();
	const FVector2D CenterB = RoomB.GetCenter();
	return FMath::Abs(CenterA.X - CenterB.X) + FMath::Abs(CenterA.Y - CenterB.Y);
}

double PMG::GetMaxRoomDistance(int32 Rows, int32 Cols, EPMGRoomSize EntranceSize, EPMGRoomSize ExitSize)
{
	double MaxDistance = -1.0;
	for (const FCenterRange& EntranceRange : GetCenterRanges(Rows, Cols, EntranceSize))
	{
		for (const FCenterRange& ExitRange : GetCenterRanges(Rows, Cols, ExitSize))
		{
			if (EntranceRange.bHasPlace && ExitRange.bHasPlace)
			{
				MaxDistance = FMath::Max(MaxDistance, GetFarthestDistance(EntranceRange, ExitRange));
			}
		}
	}
	return MaxDistance;
}

TArray<FPMGEntranceExitSizes> PMG::MakeEntranceExitSizeCombos(int32 Rows, int32 Cols, int32 QuadRoomCount, int32 DualRoomCount,
	const FPMGRoomSizeSet& EntranceRoomSizes, const FPMGRoomSizeSet& ExitRoomSizes, int32 MinEntranceExitDistance)
{
	const int32 SingleRoomCount = Rows * Cols - 4 * QuadRoomCount - 2 * DualRoomCount;

	TArray<FPMGEntranceExitSizes> SizeCombos;
	for (const EPMGRoomSize EntranceSize : AllRoomSizes)
	{
		if (!EntranceRoomSizes.Contains(EntranceSize))
		{
			continue;
		}
		for (const EPMGRoomSize ExitSize : AllRoomSizes)
		{
			if (!ExitRoomSizes.Contains(ExitSize))
			{
				continue;
			}

			FPMGEntranceExitSizes Sizes;
			Sizes.EntranceSize = EntranceSize;
			Sizes.ExitSize = ExitSize;

			// 방 수: 입구와 출구가 같은 크기면 그 크기의 방이 2개 필요하다
			const bool bEnoughRooms = Sizes.CountRoomsOfSize(EPMGRoomSize::Quad) <= QuadRoomCount
				&& Sizes.CountRoomsOfSize(EPMGRoomSize::Dual) <= DualRoomCount
				&& Sizes.CountRoomsOfSize(EPMGRoomSize::Single) <= SingleRoomCount;

			// 거리: 맵 안에서 가장 멀리 놓아도 최소 거리에 못 미치면 어떤 시도로도 안 된다
			const bool bFarEnough = GetMaxRoomDistance(Rows, Cols, EntranceSize, ExitSize) >= MinEntranceExitDistance;

			if (bEnoughRooms && bFarEnough)
			{
				SizeCombos.Add(Sizes);
			}
		}
	}
	return SizeCombos;
}

FPMGEntranceExitSizes PMG::ChooseEntranceExitSizes(const TArray<FPMGEntranceExitSizes>& SizeCombos, FRandomStream& Stream)
{
	checkf(!SizeCombos.IsEmpty(), TEXT("입구/출구 크기 조합이 없음 (UPMGSettings::Validate()가 즉시 실패로 막아야 함)"));
	return SizeCombos[Stream.RandRange(0, SizeCombos.Num() - 1)];
}

int32 PMG::GetTargetEntranceExitDistance(int32 Rows, int32 Cols, const FPMGEntranceExitSizes& Sizes, int32 MinEntranceExitDistance, int32 RouteBigRoomCount)
{
	// 방 중심 거리는 0.5 단위라서, 정수 목표가 최대 거리를 넘지 않게 내림한다
	const int32 MaxDistance = FMath::FloorToInt32(GetMaxRoomDistance(Rows, Cols, Sizes.EntranceSize, Sizes.ExitSize));
	const int32 WantedDistance = FMath::Max(MinEntranceExitDistance, RouteBigRoomCount);
	return FMath::Min(WantedDistance, MaxDistance);
}

bool PMG::ChooseEntranceAndExit(FPMGLayout& Layout, const FPMGEntranceExitSizes& Sizes, int32 TargetDistance,
	FRandomStream& Stream, FString& OutError)
{
	checkf(Layout.Rooms.Num() == Layout.Rows * Layout.Cols, TEXT("ChooseEntranceAndExit는 모든 방이 1칸인 Layout(PMG::BuildLayout)에서 불러야 한다"));

	// 1. 후보: 그 크기의 방이 놓일 수 있는 모든 자리
	const TArray<FIntRect> EntrancePlaces = GetPlacesOfSize(Layout.Rows, Layout.Cols, Sizes.EntranceSize);
	const TArray<FIntRect> ExitPlaces = GetPlacesOfSize(Layout.Rows, Layout.Cols, Sizes.ExitSize);

	// 2. 후보가 적은 쪽을 먼저 정한다 (같으면 입구 먼저)
	const bool bEntranceFirst = EntrancePlaces.Num() <= ExitPlaces.Num();
	TArray<FIntRect> FirstPlaces = bEntranceFirst ? EntrancePlaces : ExitPlaces;
	const TArray<FIntRect>& SecondPlaces = bEntranceFirst ? ExitPlaces : EntrancePlaces;

	// 3. 먼저 정하는 쪽의 후보를 섞어 차례로 본다. 짝(거리 조건을 만족하고 겹치지 않는 자리)이 있는 첫 후보를 쓴다.
	//    Stream을 넘기는 버전을 써야 시드로 재현된다 (Stream 없는 버전은 전역 난수 FMath::RandHelper64를 씀)
	Algo::RandomShuffle(FirstPlaces, Stream);

	// 실패 메시지용: 살펴본 겹치지 않는 짝 중 가장 먼 거리. 실패하면 모든 쌍을 본 뒤이므로 "가능한 가장 먼 거리"가 된다
	double FarthestDistance = -1.0;

	TArray<FIntRect> Partners;
	for (const FIntRect& FirstPlace : FirstPlaces)
	{
		Partners.Reset();
		for (const FIntRect& SecondPlace : SecondPlaces)
		{
			if (PMG::DoRectsOverlap(FirstPlace, SecondPlace))
			{
				continue;   // 입구와 출구는 서로 다른 셀에 있어야 한다
			}

			const double Distance = GetRectDistance(FirstPlace, SecondPlace);
			FarthestDistance = FMath::Max(FarthestDistance, Distance);
			if (Distance >= TargetDistance)
			{
				Partners.Add(SecondPlace);
			}
		}

		if (Partners.IsEmpty())
		{
			continue;   // 이 후보와 짝이 되는 자리가 없다 → 다음 후보
		}

		// 4. 나머지 쪽은 짝 중 무작위
		const FIntRect PartnerPlace = Partners[Stream.RandRange(0, Partners.Num() - 1)];
		const FIntRect EntrancePlace = bEntranceFirst ? FirstPlace : PartnerPlace;
		const FIntRect ExitPlace = bEntranceFirst ? PartnerPlace : FirstPlace;

		// 5. 큰 방 자리는 합친다. 1칸 자리는 이미 1칸 방이다
		TArray<FIntRect> BigRoomPlaces;
		if (Sizes.EntranceSize != EPMGRoomSize::Single)
		{
			BigRoomPlaces.Add(EntrancePlace);
		}
		if (Sizes.ExitSize != EPMGRoomSize::Single)
		{
			BigRoomPlaces.Add(ExitPlace);
		}
		MergeRooms(Layout, BigRoomPlaces);

		Layout.EntranceRoomId = Layout.GetRoomIdAt(EntrancePlace.Min);
		Layout.ExitRoomId = Layout.GetRoomIdAt(ExitPlace.Min);
		Layout.Rooms[Layout.EntranceRoomId].RoomType = RoomTypeEntrance();
		Layout.Rooms[Layout.ExitRoomId].RoomType = RoomTypeExit();
		return true;
	}

	// 6. 어느 후보에도 짝이 없다 → 시도 실패
	const FString FarthestText = (FarthestDistance < 0.0)
		? FString(TEXT("서로 겹치지 않는 두 자리가 없음"))
		: FString::Printf(TEXT("가장 먼 짝의 거리 %.1f"), FarthestDistance);
	OutError = FString::Printf(TEXT("입구/출구 자리 없음: 입구 %s 자리 %d개, 출구 %s 자리 %d개 중 거리 >= %d인 짝이 없음 (%s)"),
		*RoomSizeToString(Sizes.EntranceSize), EntrancePlaces.Num(), *RoomSizeToString(Sizes.ExitSize), ExitPlaces.Num(), TargetDistance, *FarthestText);
	return false;
}
