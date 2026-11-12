
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
	
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<UInputAction> TabAction;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<UInputAction> MapAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<UInputAction> QuitAction;

/* Actions Section */	
protected:
	void Move(const FInputActionValue& Value);
	
	void StopMove();
	
	void Look(const FInputActionValue& Value);
	
	void Jump();
	
	void ShowStatus();
	
	void StopJumping();
	
	void HoverMouse();
	
	void StopHoverMouse();
	
	void QuitMenu();
	
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

	/** 런 의상 이동 배율 (걷기와 달리기에 곱함, 기본 1이라 팀 폰은 그대로) */
	void SetMoveSpeedMultiplier(float Multiplier);
	void RefreshMoveSpeed();

	/** 걷기와 질주의 중간 속도 (의상 배율 반영). 애님 블루프린트가 질주 모션과 대시 정지 모션을 고르는 기준 */
	UFUNCTION(BlueprintPure, Category = "Movement")
	float GetSprintAnimSpeedThreshold() const;

private:
	FVector AttackInputDirection = FVector::ZeroVector;
	FVector MoveInputDirection = FVector::ZeroVector;
	
	// 기본 이동 속도 (평상시)
    UPROPERTY(EditDefaultsOnly, Category = "Setup|Movement")
    float WalkSpeed = 600.0f;
	
    // 대시 키(LeftShift) 유지 시 질주 이동 속도
    UPROPERTY(EditDefaultsOnly, Category = "Setup|Movement")
    float SprintSpeed = 1000.0f;

	float MoveSpeedMultiplier = 1.f;
	bool bSprintHeld = false;
	
/* GAS Core Section */
private:
	USoulAbilitySystemComponent* GetASC();
	
private:
	UPROPERTY()
	TObjectPtr<USoulAbilitySystemComponent> SoulAbilitySystemComponent;
	
/* UI : Quit */	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<UUserWidget> QuitMenuWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> QuitMenuWidget;
	
/* UI : DamageText */	
public:
	UFUNCTION(Client, Reliable)
	void ShowDamageText(float DamageAmount, ACharacter* TargetCharacter, bool bIsCriticalHit, int32 DamageType, bool bStackAbove = false);
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<USoulDamageTextComponent> DamageTextComponentClass;

	/** 숫자를 둘로 나눌 때 두 번째(속성) 숫자를 위로 띄우는 거리 */
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	FVector ElementTextOffset = FVector(0.f, 0.f, 45.f);

	/** 치명타일 때 위 거리 배율 (글자가 커짐) */
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	float ElementTextCritScale = 2.f;

/* UI : Map */
public:
	void ToggleMap();
};