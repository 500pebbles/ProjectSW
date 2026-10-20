// 


#include "SoulWidgetController.h"

void USoulWidgetController::BroadcastInitialValues()
{
	/* 최초에 설정되었을 때 데이터를 받아 Widget에 초기화하는 공통 기능. 
	 * 일반적으로 하위클래스에서 델리게이트를 Broadcast를 하도록 재정의.
	 * 콜백을 작성하고 바인딩하는 쪽은 해당 WidgetController를 사용하는 Widget의 "블루프린트" 측에서. */
}

void USoulWidgetController::BindCallbacksToDependencies()
{
	/* 하위클래스에서 BlueprintAssignable로 정의한 콜백함수와 원하는 Attribute변경 감지를 바인딩. */
}

void USoulWidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WCParams)
{
	/* WidgetController의 필수적인 필드를 구조체로 한번에 채워넣는 Setter. */

	PlayerController = WCParams.PlayerController;
	PlayerState = WCParams.PlayerState;
	AbilitySystemComponent = WCParams.AbilitySystemComponent;
	AttributeSet = WCParams.AttributeSet;
}
