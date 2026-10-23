
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SoulPlayerController.generated.h"

class USoulAbilitySystemComponent;
struct FGameplayTag;
class USoulInputConfig;
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
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TArray<TObjectPtr<UInputMappingContext>> InputMappingContexts;
    
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<UInputAction> JumpAction;
    
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<UInputAction> MoveAction;
    
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<UInputAction> LookAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<UInputAction> HoverMouseAction;

/* Actions Section */	
protected:
	void Move(const FInputActionValue& Value);
	
	void Look(const FInputActionValue& Value);
	
	void Jump();
	
	void StopJumping();
	
	void HoverMouse();
	
	void StopHoverMouse();
	
/* Tag Based Input Section */
private:
	void AbilityInputTagPressed(FGameplayTag InputTag);
	
	void AbilityInputTagReleased(FGameplayTag InputTag);
	
	void AbilityInputTagHeld(FGameplayTag InputTag);
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<USoulInputConfig> InputConfig;
	
/* GAS Core Section */
private:
	USoulAbilitySystemComponent* GetASC();
	
private:
	UPROPERTY()
	TObjectPtr<USoulAbilitySystemComponent> SoulAbilitySystemComponent;
};