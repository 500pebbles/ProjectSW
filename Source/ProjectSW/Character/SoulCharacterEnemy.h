
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
	
	virtual void InitAbilityActorInfo() override;


/* Combat Interface */
public:
	virtual int32 GetPlayerLevel() override;	

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	int32 Level = 1;
};
