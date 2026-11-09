#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulSpawnPatternData.generated.h"

/** 방 크기별 몹 스폰 위치 후보 (DA_SpawnPatterns, 방 로컬 cm). 방 회전을 따라간다. 막힌 곳은 NavMesh로 보정한다 */
UCLASS(BlueprintType)
class PROJECTSW_API USoulSpawnPatternData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TArray<FVector> SingleRoom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TArray<FVector> DualRoom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TArray<FVector> QuadRoom;
};
