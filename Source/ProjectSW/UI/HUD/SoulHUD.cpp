// 


#include "SoulHUD.h"

#include "UI/Widget/SoulUserWidget.h"
#include "UI/WidgetController/AttributeMenuWidgetController.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "Map/SoulMapWidget.h"
#include "Map/MapWidgetController.h"

UOverlayWidgetController* ASoulHUD::GetOverlayWidgetController(const FWidgetControllerParams& WCParams)
{
	if (OverlayWidgetController == nullptr)
	{
		OverlayWidgetController = NewObject<UOverlayWidgetController>(this, OverlayWidgetControllerClass);
		OverlayWidgetController->SetWidgetControllerParams(WCParams);
		OverlayWidgetController->BindCallbacksToDependencies();
	}		
	
	return OverlayWidgetController;
}

UMapWidgetController* ASoulHUD::GetMapWidgetController()
{
	if (MapWidgetController == nullptr)
	{
		MapWidgetController = NewObject<UMapWidgetController>(this);
		MapWidgetController->Init(GetOwningPlayerController());
	}
	
	return MapWidgetController;
}

USoulMapWidget* ASoulHUD::OpenMap()
{
	MapWidget = CreateWidget<USoulMapWidget>(GetOwningPlayerController(), MapWidgetClass);

	if (!IsValid(MapWidget))
		return nullptr;

	MapWidget->SetWidgetController(GetMapWidgetController());
	MapWidget->AddToViewport(MapWidgetZOrder);

	return MapWidget;
}

void ASoulHUD::CloseMap()
{
	if (IsValid(MapWidget))
		MapWidget->RemoveFromParent();

	MapWidget = nullptr;
}

bool ASoulHUD::IsMapOpen() const
{
	return MapWidget != nullptr;
}

void ASoulHUD::InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{		
	/* HUD와 직접 연관된 OverlayWidget의 초기화 + AddViewport 동작을 HUD에서 선언.
	   GAS의 스탯과 관련된 ViewModel 역할의 Controller들은 모두 PC, PS, ASC, AS를 보유하는 설계.
	   PC, PS, ASC, AS에 모두 접근 가능한 시점에서 초기화 <- CharacterPlayer의 InitAbilityActorInfo 시점. */
	
	// Overlay Widget 생성.
	UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), OverlayWidgetClass);
	OverlayWidget = Cast<USoulUserWidget>(Widget);
	
	// Overlay Widget Controller 생성.
	const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
	UOverlayWidgetController* WidgetController = GetOverlayWidgetController(WidgetControllerParams);
	
	// OverlayWidget에 OverlayWidgetController 등록. OnWidgetControllerSet 호출.
	OverlayWidget->SetWidgetController(WidgetController);
	
	// Broadcast Initial Values.
	WidgetController->BroadcastInitialValues();
	
	Widget->AddToViewport();
}

void ASoulHUD::InitAttributeMenu(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC,	UAttributeSet* AS)
{
	// Overlay Widget 생성.
	UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), AttributeMenuWidgetClass);
	AttributeMenuWidget = Cast<USoulUserWidget>(Widget);
	
	// Overlay Widget Controller 생성.
	const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
	UAttributeMenuWidgetController* WidgetController = GetAttributeMenuWidgetController(WidgetControllerParams);
	
	// OverlayWidget에 OverlayWidgetController 등록. OnWidgetControllerSet 호출.
	AttributeMenuWidget->SetWidgetController(WidgetController);
	
	// Broadcast Initial Values.
	WidgetController->BroadcastInitialValues();
	
	Widget->AddToViewport();
	Widget->SetVisibility(ESlateVisibility::Collapsed);
}

UAttributeMenuWidgetController* ASoulHUD::GetAttributeMenuWidgetController(const FWidgetControllerParams& WCParams)
{
	/* AttributeMenuWidgetController 자체는 HUD에서 보유. 
	 * Widget자체는 런타임중 블루프린트에서 CreateWidget으로 이루어지고 해제 시 파괴되지만
	 * 컨트롤러는 최초 생성할 때 GetAttributeMenuWidgetController에서 NewObject로 1회 생성하고 인스턴스가 유지됨.
	 * HUD의 이 함수 자체로 생성되지만 라이브러리에 정의한 Blueprint pure함수를 통해 호출. */
	
	if (AttributeMenuWidgetController == nullptr)
	{
		AttributeMenuWidgetController = NewObject<UAttributeMenuWidgetController>(this, AttributeMenuWidgetControllerClass);
		AttributeMenuWidgetController->SetWidgetControllerParams(WCParams);
		AttributeMenuWidgetController->BindCallbacksToDependencies();
	}
	
	return AttributeMenuWidgetController;
}

void ASoulHUD::ToggleAttributeMenuWidget()
{
	if (AttributeMenuWidget->GetVisibility() == ESlateVisibility::Collapsed)
	{
		AttributeMenuWidget->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		AttributeMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}
