

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

protected:
	UFUNCTION(BlueprintPure, Category = "Map")
	UTexture2D* FindRoomTypeIcon(FName RoomType) const { return RoomTypeIcons.FindRef(RoomType); }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	TMap<FName, TObjectPtr<UTexture2D>> RoomTypeIcons;
};
