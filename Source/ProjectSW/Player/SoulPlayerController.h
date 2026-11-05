
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SoulPlayerController.generated.h"

class USoulDamageTextComponent;
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
	
	void StopMove();
	
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
	
/* Combat Section */
public:
	// 공격 중 기록한 이동 입력 방향을 꺼내고 비운다. 입력이 없었으면 ZeroVector. (공격 GA가 다음 타 시작 시 회전에 사용)
	UFUNCTION(BlueprintCallable, Category = "Combat")
	FVector ConsumeAttackInputDirection();
	
	// 지금 누르고 있는 이동 입력 방향(카메라 기준). 공격 중에도 갱신되고, 키를 떼면 ZeroVector. (회피 GA가 대시 방향으로 사용)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
	FVector GetMoveInputDirection() const;
	
private:
	FVector AttackInputDirection = FVector::ZeroVector;
	FVector MoveInputDirection = FVector::ZeroVector;
	
	// 기본 이동 속도 (평상시)
    UPROPERTY(EditDefaultsOnly, Category = "Setup|Movement")
    float WalkSpeed = 600.0f;
	
    // 대시 키(LeftShift) 유지 시 질주 이동 속도
    UPROPERTY(EditDefaultsOnly, Category = "Setup|Movement")
    float SprintSpeed = 1000.0f;
	
/* GAS Core Section */
private:
	USoulAbilitySystemComponent* GetASC();
	
private:
	UPROPERTY()
	TObjectPtr<USoulAbilitySystemComponent> SoulAbilitySystemComponent;
	
/* UI : DamageText */	
public:
	UFUNCTION(Client, Reliable)
	void ShowDamageText(float DamageAmount, ACharacter* TargetCharacter, bool bIsCriticalHit, int32 DamageType);
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<USoulDamageTextComponent> DamageTextComponentClass;
};