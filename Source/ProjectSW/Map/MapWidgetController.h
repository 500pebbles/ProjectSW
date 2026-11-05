

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MapWidgetController.generated.h"

class APlayerController;
class USoulMapSubsystem;

USTRUCT(BlueprintType)
struct FSoulMapRoomView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 RoomId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	FIntRect CellBounds;
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

	TArray<FSoulMapRoomView> GetRoomViews() const;

private:
	const USoulMapSubsystem* GetMapsubsystem() const;

	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;
};
