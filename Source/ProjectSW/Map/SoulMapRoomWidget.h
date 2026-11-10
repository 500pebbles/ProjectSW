

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/SoulUserWidget.h"
#include "Map/MapWidgetController.h"
#include "SoulMapRoomWidget.generated.h"

class UButton;
class USoundBase;

/**
 * 
 */
UCLASS()
class PROJECTSW_API USoulMapRoomWidget : public USoulUserWidget
{
	GENERATED_BODY()
	
public:
	void InitRoomView(const FSoulMapRoomView& View);

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintImplementableEvent)
	void ApplyRoomView(const FSoulMapRoomView& View);

	UFUNCTION(BlueprintPure, Category = "Map")
	UTexture2D* FindRoomTypeIcon(FName RoomType) const { return RoomTypeIcons.FindRef(RoomType); }

private:
	UFUNCTION()
	void HandleWaypointClicked();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	TMap<FName, TObjectPtr<UTexture2D>> RoomTypeIcons;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	float FadeTime = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<USoundBase> TeleportSound;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> WaypointButton;

	int32 RoomId = INDEX_NONE;
};
