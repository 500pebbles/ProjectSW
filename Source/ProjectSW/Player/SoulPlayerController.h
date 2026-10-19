
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
	virtual void SetupInputComponent() override;
    
protected:    
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TArray<TObjectPtr<UInputMappingContext>> InputMappingContexts;
    
	UPROPERTY(EditDefaultsOnly, Category = "Input|Movement")
	TObjectPtr<UInputAction> JumpAction;
    
	UPROPERTY(EditDefaultsOnly, Category = "Input|Movement")
	TObjectPtr<UInputAction> MoveAction;
    
	UPROPERTY(EditDefaultsOnly, Category = "Input|Movement")
	TObjectPtr<UInputAction> LookAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input|Abilities")
	TObjectPtr<UInputAction> AbilityAction;
	
protected:
	void Move(const FInputActionValue& Value);
	
	void Look(const FInputActionValue& Value);
	
	void Jump();
	
	void StopJumping();
	
	void ActiveAbility();
};