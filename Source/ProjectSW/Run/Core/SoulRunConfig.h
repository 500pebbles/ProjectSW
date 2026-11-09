#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulRunConfig.generated.h"

class USoulDungeonFlowData;
class USoulProgressionData;
class UDataTable;
class AItemActorBase;
class USoulScoreRules;
class USoulHallOfFameRules;
class USoulEquipmentCatalog;
class USoulResultWidget;
class USoulCombatFeelData;
class USoulPresentationData;

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

	/** 런 아이템 표 (DT_RunItems) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UDataTable> ItemTable;

	/** 포션 픽업 (BP_ItemPickup)과 무기 픽업 (BP_WeaponPickup) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSubclassOf<AItemActorBase> ItemPickupClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSubclassOf<AItemActorBase> WeaponPickupClass;

	/** 의상과 무기 목록 (DA_EquipmentCatalog) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USoulEquipmentCatalog> EquipmentCatalog;

	/** 점수 정산과 랭크 (DA_ScoreRules) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Record")
	TObjectPtr<USoulScoreRules> ScoreRules;

	/** 기록 규칙 (DA_HallOfFameRules) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Record")
	TObjectPtr<USoulHallOfFameRules> HallOfFameRules;

	/** 런이 끝났을 때 결과창 (WBP_Result). 비면 바로 마을로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Record")
	TSubclassOf<USoulResultWidget> ResultWidgetClass;

	/** 타격감 (DA_CombatFeel). 비면 타격감 서브시스템이 꺼진다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feel")
	TObjectPtr<USoulCombatFeelData> CombatFeel;

	/** 연출 (DA_Presentation): 로딩 화면, 배너, 소리, 보스 인트로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TObjectPtr<USoulPresentationData> Presentation;

	/** 맵 이동 직전 페이드 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Run", meta = (ClampMin = "0", Units = "s"))
	float TravelFadeSec = 0.f;
};
