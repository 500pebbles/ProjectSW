
#pragma once

#include "CoreMinimal.h"
#include "SoulCharacterBase.h"
#include "SoulCharacterEnemy.generated.h"

UCLASS()
class PROJECTSW_API ASoulCharacterEnemy : public ASoulCharacterBase
{
	GENERATED_BODY()
	
public:
	ASoulCharacterEnemy();
	
protected:
	virtual void BeginPlay() override;
};
