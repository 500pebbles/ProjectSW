// 


#include "InventoryPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

DEFINE_LOG_CATEGORY(LogInventorySystem)

void AInventoryPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void AInventoryPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	
	if (nullptr == Subsystem) return;
	
	int32 PriorityCount = 0;
	for (auto Context : IMC_Inventory)
	{
		Subsystem->AddMappingContext(Context, PriorityCount);
		
		PriorityCount++;
	}
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	
	EnhancedInputComponent->BindAction(InventoryInteractAction, ETriggerEvent::Started, this, &AInventoryPlayerController::InventoryInteract);
}

void AInventoryPlayerController::InventoryInteract()
{
	UE_LOG(LogInventorySystem, Log, TEXT("Inventory Interact"));
}
