#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PMGTypes.generated.h"

class UWorld;

/** 방 모양 */
UENUM(BlueprintType)
enum class EPMGRoomShape : uint8
{
	/** 1칸 (1x1) */
	Single,
	/** 가로 2칸 (X축 2칸, Y축 1칸) */
	DualHorizontal,
	/** 세로 2칸 (X축 1칸, Y축 2칸) */
	DualVertical,
	/** 4칸 (2x2) */
	Quad
};

/** 방 크기. 모양(EPMGRoomShape)과 달리 가로/세로 2칸을 구분하지 않는다 (에셋 리스트가 크기별로 나뉘므로) */
enum class EPMGRoomSize : uint8
{
	/** 1칸 */
	Single,
	/** 2칸 (가로 또는 세로) */
	Dual,
	/** 4칸 (2x2) */
	Quad
};

/**
 * 방 크기(1칸/2칸/4칸) 중 어떤 것을 포함하는지 나타내는 집합.
 * 예: 한 방 타입에 에셋이 등록된 크기 → 입구/출구가 될 수 있는 방 크기 (FPMGValidationResult::EntranceRoomSizes)
 */
struct FPMGRoomSizeSet
{
	bool bSingle = false;
	bool bDual = false;
	bool bQuad = false;

	bool IsEmpty() const { return !bSingle && !bDual && !bQuad; }

	/** 선택된 크기 수 (0 ~ 3) */
	int32 CountSizes() const
	{
		int32 Count = 0;
		if (bSingle) { Count++; }
		if (bDual) { Count++; }
		if (bQuad) { Count++; }
		return Count;
	}

	/** Size 크기가 선택되어 있는지 */
	bool Contains(EPMGRoomSize Size) const
	{
		switch (Size)
		{
		case EPMGRoomSize::Single:
			return bSingle;
		case EPMGRoomSize::Dual:
			return bDual;
		case EPMGRoomSize::Quad:
			return bQuad;
		}
		return false;
	}

	/** Shape 모양인 방의 크기가 선택되어 있는지. 가로/세로 2칸은 모두 bDual */
	bool Contains(EPMGRoomShape Shape) const
	{
		switch (Shape)
		{
		case EPMGRoomShape::Single:
			return bSingle;
		case EPMGRoomShape::DualHorizontal:
		case EPMGRoomShape::DualVertical:
			return bDual;
		case EPMGRoomShape::Quad:
			return bQuad;
		}
		return false;
	}
};

/** 한 시도에서 쓸 입구 방 크기와 출구 방 크기 (2-9절 "입구/출구 배정") */
struct FPMGEntranceExitSizes
{
	EPMGRoomSize EntranceSize = EPMGRoomSize::Single;
	EPMGRoomSize ExitSize = EPMGRoomSize::Single;

	/** 입구/출구 중 Size 크기인 방 수 (0 ~ 2) */
	int32 CountRoomsOfSize(EPMGRoomSize Size) const
	{
		int32 Count = 0;
		if (EntranceSize == Size)
		{
			Count++;
		}
		if (ExitSize == Size)
		{
			Count++;
		}
		return Count;
	}
};

/** 미사용 방 처리 */
UENUM(BlueprintType)
enum class EPMGUnusedRoomPolicy : uint8
{
	DontSpawn,
	SpawnFiller
};

/** 커넥터(셀 경계 하나를 채우는 것) 종류 (3-4절) */
UENUM(BlueprintType)
enum class EPMGConnectorType : uint8
{
	/** 문: 스폰되는 두 방이 연결된 문 위치 */
	Door,
	/** 벽(개구부 막이): 그 밖의 경계 */
	Wall
};

/** 방/문/벽 에셋 하나. Level 또는 ActorClass 중 하나를 지정 */
USTRUCT(BlueprintType)
struct PROCMAPGEN_API FPMGAssetVariant
{
	GENERATED_BODY()

	/** 레벨(월드) 에셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen")
	TSoftObjectPtr<UWorld> Level;

	/** 블루프린트 액터 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen")
	TSoftClassPtr<AActor> ActorClass;

	/**
	 * 회전을 쓸 수 있는 에셋인지 (디테일 패널에 보이지 않음).
	 * 문/벽은 엣지 방향으로 놓여 회전하면 개구부에 맞지 않으므로, UPMGSettings::RefreshCanRotate()가 방 리스트는 true, 문/벽 리스트는 false로 채운다
	 */
	UPROPERTY()
	bool bCanRotate = true;

	/**
	 * 90° 단위 임의 회전 허용 (방 에셋에서만. 문/벽에서는 숨겨지고 항상 false).
	 * HideEditConditionToggle: 조건 bool이 디테일 패널에 따로 없으면 UE가 이름 옆에 그 bool의 체크박스를 붙이는데, 사용자가 bCanRotate를 바꾸지 못하게 숨긴다
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen", meta = (EditCondition = "bCanRotate", EditConditionHides, HideEditConditionToggle))
	bool bAllowRotation = true;
};

USTRUCT(BlueprintType)
struct PROCMAPGEN_API FPMGAssetSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen")
	TArray<FPMGAssetVariant> Variants;

	bool IsEmpty() const { return Variants.IsEmpty(); }
};

/** 한 방 타입의 크기별 에셋 리스트와 문 리스트 */
USTRUCT(BlueprintType)
struct PROCMAPGEN_API FPMGRoomTypeAssets
{
	GENERATED_BODY()

	/** 1칸 방 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen")
	FPMGAssetSet SingleRooms;

	/** 2칸 방 (가로 X축 2칸으로 제작) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen")
	FPMGAssetSet DualRooms;

	/** 4칸 방 (2x2) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen")
	FPMGAssetSet QuadRooms;

	/**
	 * 이 타입 방의 문 (비밀방의 폭탄문 등). 비어 있으면 전역 Doors를 씀.
	 * 문 양쪽 방 중 들어가는 쪽 방의 타입을 먼저 보고, 비어 있으면 반대쪽 방의 타입을 본다
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen")
	FPMGAssetSet Doors;

	/** Size 크기 방의 에셋 리스트 (1칸 SingleRooms, 2칸 DualRooms, 4칸 QuadRooms) */
	const FPMGAssetSet& GetRoomAssets(EPMGRoomSize Size) const
	{
		switch (Size)
		{
		case EPMGRoomSize::Dual:
			return DualRooms;
		case EPMGRoomSize::Quad:
			return QuadRooms;
		default:
			return SingleRooms;
		}
	}

	/** 방 에셋이 하나라도 등록된 크기 (이 타입이 될 수 있는 방 크기, 2-9절 "있는 것만 사용"). 문 리스트는 보지 않는다 */
	FPMGRoomSizeSet GetRegisteredRoomSizes() const
	{
		FPMGRoomSizeSet Sizes;
		Sizes.bSingle = !SingleRooms.IsEmpty();
		Sizes.bDual = !DualRooms.IsEmpty();
		Sizes.bQuad = !QuadRooms.IsEmpty();
		return Sizes;
	}
};

/** 사용자 추가 방 타입 */
USTRUCT(BlueprintType)
struct PROCMAPGEN_API FPMGCustomRoomType
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen")
	FName TypeName;

	/** 이 타입으로 배정할 방 수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen", meta = (ClampMin = "0"))
	int32 Count = 0;

	/** true면 추가방에서만 배정 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen")
	bool bExtraRoom = false;

	/** true면 입구방과 문으로 바로 이어진 방에는 배정하지 않음 (예: 시작하자마자 들어가는 상점/보물방을 막기) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen")
	bool bAvoidDoorToEntrance = false;

	/** true면 출구방과 문으로 바로 이어진 방에는 배정하지 않음 (예: 보스방(출구)에 딸린 상점/보물방을 막기) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen")
	bool bAvoidDoorToExit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcMapGen")
	FPMGRoomTypeAssets Assets;

	/**
	 * 추가방용 큰 방을 예약해야 하는 타입인지 (2-9절 "추가방용 큰 방 예약"): 추가방에만 배정되고, 1칸 에셋이 없고, 큰 방(2칸/4칸) 에셋은 있음.
	 * 추가방은 대부분 1칸이라 이런 타입은 추가방 중에 큰 방이 있어야 배정될 수 있다.
	 * UPMGSettings::Validate()가 예약 대상을 고를 때와 Phase 7 PMG::AssignCustomRoomTypes가 처리 순서를 정할 때 쓴다
	 */
	bool NeedsReservedBigRoom() const
	{
		const FPMGRoomSizeSet Sizes = Assets.GetRegisteredRoomSizes();
		return bExtraRoom && !Sizes.bSingle && (Sizes.bDual || Sizes.bQuad);
	}
};

namespace PMG
{
	/** 기본 방 타입 이름 (예약어) */
	inline FName RoomTypeEntrance() { static const FName Name(TEXT("Entrance")); return Name; }
	inline FName RoomTypeExit() { static const FName Name(TEXT("Exit")); return Name; }
	inline FName RoomTypeStandard() { static const FName Name(TEXT("Standard")); return Name; }

	/** 모양의 크기 (가로/세로 2칸은 모두 Dual) */
	inline EPMGRoomSize GetRoomSize(EPMGRoomShape Shape)
	{
		switch (Shape)
		{
		case EPMGRoomShape::DualHorizontal:
		case EPMGRoomShape::DualVertical:
			return EPMGRoomSize::Dual;
		case EPMGRoomShape::Quad:
			return EPMGRoomSize::Quad;
		default:
			return EPMGRoomSize::Single;
		}
	}

	/** 모양이 차지하는 칸 수 (X축 칸 수, Y축 칸 수). 예: 가로 2칸 (2, 1), 세로 2칸 (1, 2) */
	inline FIntPoint GetShapeCellCount(EPMGRoomShape Shape)
	{
		switch (Shape)
		{
		case EPMGRoomShape::DualHorizontal:
			return FIntPoint(2, 1);
		case EPMGRoomShape::DualVertical:
			return FIntPoint(1, 2);
		case EPMGRoomShape::Quad:
			return FIntPoint(2, 2);
		default:
			return FIntPoint(1, 1);
		}
	}
}
