
#pragma once

#include "CoreMinimal.h"
#include "SoulCharacterBase.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "SoulCharacterEnemy.generated.h"

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


/* Combat Interface */
public:
	virtual int32 GetPlayerLevel() override;	

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	int32 Level = 1;
	
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
