
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SoulGameModeBase.generated.h"

class USoulEnemyTypeInfo;

UCLASS()
class PROJECTSW_API ASoulGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<USoulEnemyTypeInfo> EnemyTypeInfo;
};