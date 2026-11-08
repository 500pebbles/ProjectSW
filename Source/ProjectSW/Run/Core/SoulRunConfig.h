#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulRunConfig.generated.h"

class USoulDungeonFlowData;
class USoulProgressionData;

/**
 * 런 진입점 (DA_RunConfig). BP_RunGameMode와 이후 BP_TitleGameMode의 USoulRunGameComponent가 하드 참조해 쿡 루트가 된다.
 * 다른 런 데이터(연출, 진행, 장비, 점수)는 그 단계에서 여기에 칸이 붙는다.
 */
UCLASS(BlueprintType)
class PROJECTSW_API USoulRunConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 마을 맵 (L_Town) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Run")
	TSoftObjectPtr<UWorld> TownMap;

	/** 던전 맵 (L_Dungeon) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Run")
	TSoftObjectPtr<UWorld> DungeonMap;

	/** 층 배열과 전환 값 (DA_DungeonFlow) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Run")
	TObjectPtr<USoulDungeonFlowData> DungeonFlow;

	/** 레벨과 경험치 (DA_Progression) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Run")
	TObjectPtr<USoulProgressionData> Progression;
};
