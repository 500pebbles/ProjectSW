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
	
	// 인터페이스를 통해 의존성 분리
	// InputComponent에서 인터페이스를 상속해 델리게이트의 정의를 반환하고, 여기서 반환받은 정의에 이벤트 구독
	TArray<UActorComponent*> InventoryInputComponent = GetOwner()->GetComponentsByInterface(UInventoryInputInterface::StaticClass());
	for (UActorComponent* InventoryInput : InventoryInputComponent)
	{
		IInventoryInputInterface* InputInterface = Cast<IInventoryInputInterface>(InventoryInput);
		if (InputInterface)
		{
			InputInterface->GetOnToggleInventoryUI().AddDynamic(this, &UInventoryComponent::ToggleInventoryUI);
			InputInterface->GetOnCollectItem().AddDynamic(this, &UInventoryComponent::CollectItem);
		}
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
