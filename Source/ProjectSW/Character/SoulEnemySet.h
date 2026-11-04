

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulEnemySet.generated.h"

class ASoulCharacterEnemy;

/**
 * 
 */
UCLASS()
class PROJECTSW_API USoulEnemySet : public UDataAsset
{
	GENERATED_BODY()
	
public:
	const TArray<TSubclassOf<ASoulCharacterEnemy>>& GetEnemyClasses() const { return EnemyClasses; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TArray<TSubclassOf<ASoulCharacterEnemy>> EnemyClasses;
};
