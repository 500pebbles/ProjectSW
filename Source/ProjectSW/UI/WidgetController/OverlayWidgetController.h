// 

#pragma once

#include "CoreMinimal.h"
#include "SoulWidgetController.h"
#include "OverlayWidgetController.generated.h"

struct FOnAttributeChangeData;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangedSignature, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxHealthChangedSignature, float, NewMaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChangedSignature, float, NewStamina);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxStaminaChangedSignature, float, NewMaxStamina);

/* OverlayWidget의 셋업 플로우.
 * HUD에서 관리하는 메인 위젯 OverlayWidget은 HUD의 InitOverlay 함수로 생성 + WidgetController 부착.
 * WidgetController의 Params가 모두 준비되는 PlayerCharacter의 InitAbilityActorInfo시점에서 초기화. */

UCLASS(BlueprintType, Blueprintable)
class PROJECTSW_API UOverlayWidgetController : public USoulWidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	
	virtual void BindCallbacksToDependencies() override;
	
/* Delegate Instances */	
	UPROPERTY(BlueprintAssignable)
	FOnHealthChangedSignature OnHealthChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnMaxHealthChangedSignature OnMaxHealthChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnStaminaChangedSignature OnStaminaChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnMaxStaminaChangedSignature OnMaxStaminaChanged;	
	
/*  Callbacks */
protected:
	void HealthChanged(const FOnAttributeChangeData& Data) const;
	void MaxHealthChanged(const FOnAttributeChangeData& Data) const;
	void StaminaChanged(const FOnAttributeChangeData& Data) const;
	void MaxStaminaChanged(const FOnAttributeChangeData& Data) const;
};