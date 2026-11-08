#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SoulItemLibrary.generated.h"

class AItemActorBase;

/** 런 아이템 도우미 */
UCLASS()
class PROJECTSW_API USoulItemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 아이템 행으로 픽업을 바닥(NavMesh 투영)에 스폰하고, 던전이면 층 액터로 등록한다 */
	UFUNCTION(BlueprintCallable, Category = "Soul|Item", meta = (WorldContext = "WorldContextObject"))
	static AActor* SpawnPickup(UObject* WorldContextObject, TSubclassOf<AItemActorBase> PickupClass, const FDataTableRowHandle& ItemRow, const FVector& Location, const FVector& NavExtent, float FallbackForward);
};
