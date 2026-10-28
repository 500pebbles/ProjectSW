
#pragma once

#include "CoreMinimal.h"
#include "SoulCharacterBase.h"
#include "GAS/Data/SoulEnemyTypeInfo.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "SoulCharacterEnemy.generated.h"

struct FGameplayTag;
class UWidgetComponent;

UCLASS()
class PROJECTSW_API ASoulCharacterEnemy : public ASoulCharacterBase
{
	GENERATED_BODY()
	
public:
	ASoulCharacterEnemy();
	
protected:
	virtual void BeginPlay() override;
	
	virtual void InitAbilityActorInfo() override;
	
	virtual void InitializeDefaultAttributes() const override;


/* Combat Interface */
public:
	virtual int32 GetPlayerLevel() override;	

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
};
