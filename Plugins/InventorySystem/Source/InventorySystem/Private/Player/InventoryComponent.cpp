// 


#include "Player/InventoryComponent.h"

#include "Blueprint/UserWidget.h"
#include "Player/InventoryInputComponent.h"
#include "UI/Inventory/GridInventoryWidget.h"
#include "UI/Inventory/InventoryBase.h"
#include "GameFramework/Character.h"


// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InventoryUIClass = UGridInventoryWidget::StaticClass();
}


// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	APawn* PlayerPawn = Cast<APawn>(GetOwner());
	if (IsValid(PlayerPawn))
	{
		LocalGameController = Cast<APlayerController>(PlayerPawn->GetController());
		
		CreateInventoryUIWidget();
	}
	
	UInventoryInputComponent* InventoryInputComponent = GetOwner()->FindComponentByClass<UInventoryInputComponent>();
	if (IsValid(InventoryInputComponent))
	{
		InventoryInputComponent->OnToggleInventoryUI.AddDynamic(this, &UInventoryComponent::ToggleInventoryUI);
	}
}

void UInventoryComponent::CreateInventoryUIWidget()
{
	InventoryUI = CreateWidget<UInventoryBase>(LocalGameController, InventoryUIClass);
	
	if (IsValid(InventoryUI))
	{
		InventoryUI->AddToViewport();
	}
}

void UInventoryComponent::ToggleInventoryUI() const
{
	InventoryUI->ToggleInventoryUI();
}
