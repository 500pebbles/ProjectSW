
#pragma once

#include "CoreMinimal.h"
#include "SoulCharacterBase.h"
#include "SoulCharacterPlayer.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS()
class PROJECTSW_API ASoulCharacterPlayer : public ASoulCharacterBase
{
	GENERATED_BODY()
	
public:
	ASoulCharacterPlayer();
	
	virtual void BeginPlay() override;
	
public:
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void OnRep_PlayerState() override;
	
/* GAS Section */
protected:
	virtual void InitAbilityActorInfo() override;
	

/* Combat Interface */
public:
	virtual int32 GetCharacterLevel() override;	
	
/* Component Section */
protected:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;
	
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;
	
};
