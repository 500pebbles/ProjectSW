// 


#include "Player/InventoryComponent.h"

#include "Blueprint/UserWidget.h"
#include "Player/InventoryInputComponent.h"
#include "UI/Inventory/GridInventoryWidget.h"
#include "UI/Inventory/InventoryBase.h"
#include "Item/ItemComponent.h"

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
		InventoryInputComponent->OnCollectItem.AddDynamic(this, &UInventoryComponent::CollectItem);
	}
	
	// 인벤토리 데이터 초기화
	//InventoryData.InitializeArray();
}

void UInventoryComponent::CreateInventoryUIWidget()
{
	InventoryUI = CreateWidget<UInventoryBase>(LocalGameController, InventoryUIClass);
	
	if (IsValid(InventoryUI))
	{
		InventoryUI->AddToViewport(5);
	}
}

void UInventoryComponent::ToggleInventoryUI()
{
	InventoryUI->ToggleInventoryUI();
	
	FInputModeGameAndUI InventoryInputMode;
	FInputModeGameOnly GameInputMode;
	
	// 인벤토리 UI 닫기
	if (bIsInventoryUIOpen)
	{
		LocalGameController->SetInputMode(GameInputMode);
		LocalGameController->SetShowMouseCursor(false);
		bIsInventoryUIOpen = false;
	}
	// 인벤토리 UI 오픈
	else
	{
		LocalGameController->SetInputMode(InventoryInputMode);
		LocalGameController->SetShowMouseCursor(true);
		bIsInventoryUIOpen = true;
	}
}

void UInventoryComponent::CollectItem(AActor* TargetItem)
{
	UItemComponent* TargetItemComponent = TargetItem->GetComponentByClass<UItemComponent>();
	InventoryData.AddItemData(TargetItemComponent);
}
