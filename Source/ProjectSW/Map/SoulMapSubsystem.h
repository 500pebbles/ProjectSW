

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/PMGResult.h"
#include "SoulMapSubsystem.generated.h"

class APMGMapBuilder;
class ASoulPackedLevelActor;

/**
 * 
 */
UCLASS()
class PROJECTSW_API USoulMapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	UFUNCTION()
	void HandleGenerationFinished(const FPMGResult& Result, const TArray<FString>& SpawnErrors);

	void HandleRoomEntered(ASoulPackedLevelActor* Room);

private:
	UPROPERTY()
	TWeakObjectPtr<APMGMapBuilder> MapBuilder;

	UPROPERTY()
	FPMGResult MapResult;

	UPROPERTY()
	TMap<int32, TObjectPtr<ASoulPackedLevelActor>> Rooms;
};
