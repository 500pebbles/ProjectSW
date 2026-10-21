// 


#include "InventoryPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InventoryHUDWidget.h"

DEFINE_LOG_CATEGORY(LogInventorySystem)

void AInventoryPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// 컨트롤러의 InputMappingContext에 생성한 인벤토리 맵핑 컨텍스트 추가
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	
	if (nullptr == Subsystem) return;
	
	int32 PriorityCount = 0;
	for (auto Context : IMC_Inventory)
	{
		Subsystem->AddMappingContext(Context, PriorityCount);
		
		PriorityCount++;
	}
	
	CreateInventoryHUDWidget();
}

void AInventoryPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	
	EnhancedInputComponent->BindAction(InventoryInteractAction, ETriggerEvent::Started, this, &AInventoryPlayerController::InventoryInteract);
}

void AInventoryPlayerController::InventoryInteract()
{
	UE_LOG(LogInventorySystem, Log, TEXT("Inventory Interact"));
}

void AInventoryPlayerController::CreateInventoryHUDWidget()
{
	// 이렇게 직접 할당할 수도 있지만 EditDefaultsOnly이므로 에디터에서 직접 할당해야함
	// InventoryHUDWidgetClass = UInventoryHUDWidget::StaticClass();
	
	InventoryHUDWidget = CreateWidget<UInventoryHUDWidget>(this, InventoryHUDWidgetClass);
	
	if (IsValid(InventoryHUDWidget))
	{
		InventoryHUDWidget->AddToViewport();
	}
}
