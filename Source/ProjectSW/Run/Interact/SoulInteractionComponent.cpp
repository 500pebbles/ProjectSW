#include "Run/Interact/SoulInteractionComponent.h"
#include "Run/Interact/SoulInteractableComponent.h"
#include "Utility/SoulLog.h"

#include "Components/Widget.h"
#include "GameFramework/Pawn.h"
#include "Inventory/Interface/InventoryInputInterface.h"
#include "Inventory/Player/InventoryComponent.h"
#include "Inventory/Player/InventoryInputComponent.h"
#include "Inventory/UI/InventoryHUDWidget.h"
#include "Item/ItemComponent.h"
#include "TimerManager.h"

namespace SoulInteraction
{
	/** 플러그인 private 칸을 이름으로 읽는다 (플러그인 파일을 고치지 않기 위해) */
	template <typename T>
	T* ReadObjectProperty(const UObject* Object, FName Name)
	{
		const FObjectProperty* Property = Object ? FindFProperty<FObjectProperty>(Object->GetClass(), Name) : nullptr;
		return Property ? Cast<T>(Property->GetObjectPropertyValue_InContainer(Object)) : nullptr;
	}
}

USoulInteractionComponent::USoulInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

USoulInteractionComponent* USoulInteractionComponent::Find(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<USoulInteractionComponent>() : nullptr;
}

void USoulInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !GetWorld()->IsGameWorld())
		return;

	// 플랜 2.3: 컨트롤러가 있으면 다음 틱, 없으면(지연 스폰) 컨트롤러가 생긴 다음 틱
	if (Pawn->GetController())
	{
		HandleControllerChanged(Pawn, nullptr, Pawn->GetController());
	}
	else
	{
		Pawn->ReceiveControllerChangedDelegate.AddUniqueDynamic(this, &ThisClass::HandleControllerChanged);
	}
}

void USoulInteractionComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	if (!NewController || bInitScheduled || Inventory)
		return;
	bInitScheduled = true;
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::InitializeInventory);
}

void USoulInteractionComponent::InitializeInventory()
{
	AActor* Owner = GetOwner();
	if (!Owner || !InventoryClass || !InventoryInputClass || Inventory)
		return;

	// 둘 다 만든 뒤 인벤토리부터 등록한다. 등록이 InitializeComponent와 BeginPlay를 바로 부르는데,
	// 인벤토리 InitializeComponent가 입력 컴포넌트의 델리게이트에 묶이고, 입력 BeginPlay가 퀵슬롯 키 글자를 방송하기 때문이다
	Inventory = NewObject<UInventoryComponent>(Owner, InventoryClass, TEXT("SoulInventory"));
	InventoryInput = NewObject<UInventoryInputComponent>(Owner, InventoryInputClass, TEXT("SoulInventoryInput"));
	Inventory->RegisterComponent();
	InventoryInput->RegisterComponent();

	// F 라우터: 인벤토리가 직접 받던 줍기와 프롬프트를 이 컴포넌트가 받아 나눈다
	if (IInventoryInputInterface* Input = Cast<IInventoryInputInterface>(InventoryInput))
	{
		Input->GetOnCollectItem().RemoveDynamic(Inventory.Get(), &UInventoryComponent::CollectItem);
		Input->GetOnCollectItem().AddUniqueDynamic(this, &ThisClass::HandleCollect);
		Input->GetOnTraceItemChanged().Remove(Inventory.Get(), TEXT("HandleTraceItemChanged"));
		Input->GetOnTraceItemChanged().AddUniqueDynamic(this, &ThisClass::HandleTraceChanged);
		Input->GetToggleEquipmentUI().AddUniqueDynamic(this, &ThisClass::HandleEquipmentToggled);
	}

	SW_LOG("[INTERACT] 인벤토리 준비 완료 %s", *Owner->GetName());
}

void USoulInteractionComponent::HandleCollect(AActor* Target)
{
	if (!IsValid(Target))
		return;

	if (USoulInteractableComponent* Interactable = Target->FindComponentByClass<USoulInteractableComponent>())
	{
		Interactable->Interact(Cast<APawn>(GetOwner()));
		return;
	}

	if (Target->FindComponentByClass<UItemComponent>() && Inventory)
	{
		Inventory->CollectItem(Target);
		// 추가에 성공하면 플러그인이 액터를 지운다
		if (!IsValid(Target) || Target->IsActorBeingDestroyed())
		{
			OnItemCollected.Broadcast(Target);
		}
	}
}

void USoulInteractionComponent::HandleTraceChanged(bool bHasTarget, FKey InputKey, AActor* TargetActor)
{
	if (!Inventory)
		return;

	const USoulInteractableComponent* Interactable = IsValid(TargetActor) ? TargetActor->FindComponentByClass<USoulInteractableComponent>() : nullptr;
	if (bHasTarget && Interactable)
	{
		UInventoryHUDWidget* HUD = SoulInteraction::ReadObjectProperty<UInventoryHUDWidget>(Inventory, TEXT("InventoryHUDWidget"));
		if (HUD)
		{
			if (Interactable->bEnabled)
			{
				HUD->ShowInteractionMessage(InputKey, Interactable->PromptText);
			}
			else
			{
				HUD->HideInteractionMessage();
			}
		}
		return;
	}

	// 아이템이나 대상 없음은 플러그인 처리 그대로 (protected라 이름으로 부른다)
	if (UFunction* Function = Inventory->FindFunction(TEXT("HandleTraceItemChanged")))
	{
		struct FParams
		{
			bool bHasTarget;
			FKey InputKey;
			AActor* TargetItemActor;
		} Params{ bHasTarget, InputKey, TargetActor };
		Inventory->ProcessEvent(Function, &Params);
	}
}

void USoulInteractionComponent::HandleEquipmentToggled()
{
	// 플러그인에 장비창 열림 조회가 없어 토글을 미러한다
	bEquipmentOpen = !bEquipmentOpen;
}

void USoulInteractionComponent::CloseInventoryUI()
{
	if (!Inventory)
		return;

	const UWidget* InventoryUI = SoulInteraction::ReadObjectProperty<UWidget>(Inventory, TEXT("InventoryUI"));
	if (InventoryUI && InventoryUI->IsVisible())
	{
		Inventory->ToggleInventoryUI();
	}
	if (bEquipmentOpen)
	{
		if (UFunction* Function = Inventory->FindFunction(TEXT("ToggleEquipmentUI")))
		{
			Inventory->ProcessEvent(Function, nullptr);
		}
		bEquipmentOpen = false;
	}
}
