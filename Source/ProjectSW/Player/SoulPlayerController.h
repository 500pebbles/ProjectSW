
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SoulPlayerController.generated.h"

struct FInputActionValue;
class UInputAction;
class UInputMappingContext;

UCLASS()
class PROJECTSW_API ASoulPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASoulPlayerController();
	
protected:
	virtual void BeginPlay() override;
	
	virtual void SetupInputComponent() override;

/* Input Section */	
protected:    
	UPROPERTY(EditDefaultsOnly, Category = "Setup|Input")
	TArray<TObjectPtr<UInputMappingContext>> InputMappingContexts;
    
	UPROPERTY(EditDefaultsOnly, Category = "Setup|Input")
	TObjectPtr<UInputAction> JumpAction;
    
	UPROPERTY(EditDefaultsOnly, Category = "Setup|Input")
	TObjectPtr<UInputAction> MoveAction;
    
	UPROPERTY(EditDefaultsOnly, Category = "Setup|Input")
	TObjectPtr<UInputAction> LookAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Setup|Input")
	TObjectPtr<UInputAction> AbilityAction;

/* Actions Section */	
protected:
	void Move(const FInputActionValue& Value);
	
	void Look(const FInputActionValue& Value);
	
	void Jump();
	
	void StopJumping();
	
	void ActiveAbility();
};