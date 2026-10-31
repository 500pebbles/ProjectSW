
#pragma once

#include "CoreMinimal.h"
#include "SoulCharacterBase.h"
#include "GAS/Data/SoulEnemyTypeInfo.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "SoulCharacterEnemy.generated.h"

class UBehaviorTree;
class ASoulAIController;
struct FGameplayTag;
class UWidgetComponent;

UCLASS()
class PROJECTSW_API ASoulCharacterEnemy : public ASoulCharacterBase
{
	GENERATED_BODY()
	
public:
	ASoulCharacterEnemy();
	
	virtual void PossessedBy(AController* NewController) override;

protected:
	virtual void BeginPlay() override;
	
	virtual void InitAbilityActorInfo() override;
	
	virtual void InitializeDefaultAttributes() const override;


/* Combat Section */
public:
	virtual int32 GetCharacterLevel() override;	
	
	virtual void Die() override;
	
	virtual void SetCombatTarget_Implementation(AActor* CombatTargetActor) override;
	
	virtual AActor* GetCombatTarget_Implementation() const override;
	
public:
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> CombatTarget;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	int32 Level = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	EEnemyType EnemyType = EEnemyType::EnemyType_01;
	
/* Hit React Section */
public:
	void OnHitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	
public:
	UPROPERTY(BlueprintReadOnly)
	bool bHitReacting = false;
	
/* Character Movement Section */
public:
	UPROPERTY(BlueprintReadOnly)
	float BaseWalkSpeed = 250.f;
	
/* UI HealthBar Section */
public:
	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnHealthChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnMaxHealthChanged;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> HealthBar;
	
/* AI Section */	
protected:
	UPROPERTY()
	TObjectPtr<ASoulAIController> AIController;
	
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<UBehaviorTree> BehaviorTree;
};
