

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SoulRoomSpawnPointComponent.generated.h"

class USoulEnemySet;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTSW_API USoulRoomSpawnPointComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USoulRoomSpawnPointComponent();		

	const USoulEnemySet* GetEnemySet() const { return EnemySet.Get(); }
	void SetEnemySet(USoulEnemySet* InEnemySet) { EnemySet = InEnemySet; } // 런 층별 세트 주입

private:
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<USoulEnemySet> EnemySet;
};
