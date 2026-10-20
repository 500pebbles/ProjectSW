// 

#pragma once

#include "CoreMinimal.h"
#include "SoulWidgetController.h"
#include "OverlayWidgetController.generated.h"

struct FOnAttributeChangeData;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeChangedSignature, float, NewValue);

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
	
/* Delegate Instances (Callbacks are Lambda) */	
	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnHealthChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnMaxHealthChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnStaminaChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnMaxStaminaChanged;	
};