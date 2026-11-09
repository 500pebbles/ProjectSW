

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MapWidgetController.generated.h"

class APlayerController;
class USoulMapSubsystem;

// 미방문방은 위젯 자체를 만들지 않기 때문에 해당 상태는 없다.
UENUM(BlueprintType)
enum class ESoulMapRoomState : uint8
{
	Adjacent,
	Visited,
	Current
};

USTRUCT(BlueprintType)
struct FSoulMapRoomView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 RoomId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	FIntRect CellBounds;

	UPROPERTY(BlueprintReadOnly)
	ESoulMapRoomState State = ESoulMapRoomState::Adjacent;

	UPROPERTY(BlueprintReadOnly)
	FName RoomType;

	UPROPERTY(BlueprintReadOnly)
	bool bHasWaypoint = false;
};

USTRUCT(BlueprintType)
struct FSoulMapDoorView
{
	GENERATED_BODY()

	// 문이 놓이는 경계 중점의 셀 좌표 (예: (3.0, 2.5))
	UPROPERTY(BlueprintReadOnly)
	FVector2D Cell = FVector2D::ZeroVector;

	// 두 셀이 X 방향으로 이웃해있으면 true (세로 경계선)
	UPROPERTY(BlueprintReadOnly)
	bool bCrossesX = false;
};

/**
 * 
 */
UCLASS(BlueprintType)
class PROJECTSW_API UMapWidgetController : public UObject
{
	GENERATED_BODY()
	
public:
	void Init(APlayerController* InPlayerController);

	bool GetUsedCellBounds(FIntRect& OutBounds) const;

	bool RequestTeleport(int32 RoomId);

	TArray<FSoulMapRoomView> GetRoomViews() const;
	TArray<FSoulMapDoorView> GetDoorViews() const;

private:
	const USoulMapSubsystem* GetMapSubsystem() const;

	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;
};
