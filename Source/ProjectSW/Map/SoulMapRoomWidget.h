

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/SoulUserWidget.h"
#include "Map/MapWidgetController.h"
#include "SoulMapRoomWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTSW_API USoulMapRoomWidget : public USoulUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent)
	void ApplyRoomView(const FSoulMapRoomView& View);
};
