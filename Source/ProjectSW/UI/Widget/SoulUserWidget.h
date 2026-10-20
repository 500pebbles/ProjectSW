
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SoulUserWidget.generated.h"

/* SoulUserWidget 자체는 보편적으로 사용하는 위젯 타입. 
 * 플레이어의 GAS 데이터와 관련된 위젯은 PC, PS, ASC, AS를 보유하는 WidgetController와 함께 사용.
 * WidgetController는 선택사항. 이 경우 MVVM 구조의 사용하는 컨트롤러 기반 위젯 */

UCLASS()
class PROJECTSW_API USoulUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
/* Widget Controller Section */	
public:
	UFUNCTION(BlueprintCallable)
	void SetWidgetController(UObject* InWidgetController);
	
public:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UObject> WidgetController;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void OnWidgetControllerSet();
};