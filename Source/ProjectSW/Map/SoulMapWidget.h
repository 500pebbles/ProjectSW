

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/SoulUserWidget.h"
#include "SoulMapWidget.generated.h"

class UCanvasPanel;
class UTextBlock;
class USoulMapRoomWidget;
class UButton;

/**
 * 
 */
UCLASS()
class PROJECTSW_API USoulMapWidget : public USoulUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void RequestClose();

	FVector2D CellToPx(const FVector2D& Cell) const;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	FKey ToggleKey = EKeys::M;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> RoomCanvas;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CurrentFloorText;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<USoulMapRoomWidget> RoomWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	FVector2D MapFrameSize = FVector2D(800.0, 800.0);

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	double MaxCellPx = 120.0;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	double RoomGapPx = 2.0;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<UUserWidget> DoorWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	double DoorLengthPx = 28.0;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	double DoorThicknessPx = 10.0;

	FVector2D UsedCenter = FVector2D::ZeroVector;
	double CellPx = 0.0;
};
