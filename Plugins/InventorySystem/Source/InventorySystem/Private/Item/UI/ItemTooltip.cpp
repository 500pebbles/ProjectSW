// 


#include "Item/UI/ItemTooltip.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Item/Data/ItemData.h"

void UItemTooltip::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	SetVisibility(ESlateVisibility::Collapsed);
}

void UItemTooltip::InitItemTooltip(UItemData* InItemData)
{
	if (!InItemData)
	{
		return;
	}
	
	UTexture2D* Icon = InItemData->GetItemIcon();
	if (Icon)
	{
		IconImage->SetBrushFromTexture(Icon);
	}
	Text_ItemName->SetText(InItemData->GetItemName());
	Text_ItemDescription->SetText(InItemData->GetItemDescription());
	//Text_ItemEffect = 
	bIsSet = true;
}

void UItemTooltip::UpdateTooltipPosition(const FGeometry& ItemUIGeometry)
{
	// 아이템 툴팁 UI의 기준이 아이템 UI의 될 좌표 설정(기준 위치인 좌상단과 크기 기준이 될 우하단)
	FVector2D TopLeft, BottomRight, Unused;
	USlateBlueprintLibrary::LocalToViewport(this, ItemUIGeometry, FVector2D::ZeroVector, TopLeft, Unused);
	USlateBlueprintLibrary::LocalToViewport(this, ItemUIGeometry, ItemUIGeometry.GetLocalSize(), BottomRight, Unused);
	
	// 위치 설정은 마우스를 가져다 댈 때 최초 1회 실행될 예정이다.
	// 다만 Render가 Tick보다 뒤쪽에 실행되기 때문에 본래라면 크기 설정도 다음 프레임에 진행됨
	// ForceLayoutPrepass는 위젯의 desiredSize를 가져다가 현재 Tick 실행전에 즉시 처리하도록 함
	ForceLayoutPrepass();
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	const float DPIScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D TooltipSize = GetDesiredSize() * DPIScale;
	
	// 오른쪽이 잘리는지 확인
	const bool bOverflowRight = BottomRight.X + TooltipSize.X > ViewportSize.X;
	// 아래쪽이 잘리는지 확인
	const bool bOverflowBottom = BottomRight.Y + TooltipSize.Y > ViewportSize.Y;
	
	FVector2D UIPosition;
	// 오른쪽이 잘리면 UI를 왼쪽으로 띄움
	UIPosition.X = bOverflowRight ? TopLeft.X - TooltipSize.X : BottomRight.X;
	// 아래쪽이 잘리면 UI를 위쪽으로 띄움
	UIPosition.Y = bOverflowBottom ? TopLeft.Y - TooltipSize.Y : BottomRight.Y;
	
	SetPositionInViewport(UIPosition, true);
}
