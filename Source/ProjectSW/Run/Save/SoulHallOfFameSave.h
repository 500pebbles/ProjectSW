#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Run/Save/SoulHallOfFameTypes.h"
#include "SoulHallOfFameSave.generated.h"

/** 프로필과 명예의 전당 (슬롯 SoulProfile, 백업 SoulProfile_Bak). 던전 안 상태는 저장하지 않는다 */
UCLASS()
class PROJECTSW_API USoulHallOfFameSave : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY(SaveGame)
	int32 SaveVersion = CurrentVersion;

	UPROPERTY(SaveGame)
	FString LastPlayerName;

	UPROPERTY(SaveGame)
	FName LastCostumeId;

	UPROPERTY(SaveGame)
	TArray<FSoulHallOfFameEntry> HallOfFame;

	UPROPERTY(SaveGame)
	int32 TotalRuns = 0;

	UPROPERTY(SaveGame)
	int32 TotalClears = 0;

	UPROPERTY(SaveGame)
	int32 BestFloor = 0;

	/** 마지막 칸. 저장 때 EndMarkerValue를 넣고, 읽은 값이 다르면 잘린 파일로 본다 (기본값 0) */
	UPROPERTY(SaveGame)
	int32 EndMarker = 0;

	static constexpr int32 EndMarkerValue = 0x50574B21;
};
