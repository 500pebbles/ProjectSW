
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
	
/* Component Section */
protected:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;
	
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;
};
