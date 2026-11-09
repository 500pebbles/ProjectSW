#include "Run/Interact/SoulInteractionComponent.h"
#include "Run/Interact/SoulInteractableComponent.h"
#include "Run/Equipment/SoulEquipmentComponent.h"
#include "Run/Equipment/SoulEquipmentCatalog.h"
#include "Run/Core/SoulRunSubsystem.h"
#include "Run/Interact/SoulItemLibrary.h"
#include "Run/UI/SoulKeyHintWidget.h"
#include "Utility/SoulLog.h"

#include "Components/Widget.h"
#include "GameFramework/Pawn.h"
#include "Inventory/Interface/InventoryInputInterface.h"
#include "Inventory/Player/InventoryComponent.h"
#include "Inventory/Player/InventoryInputComponent.h"
#include "Inventory/UI/InventoryHUDWidget.h"
#include "Item/ItemComponent.h"
#include "TimerManager.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"

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

	if (const UItemComponent* ItemComp = Target->FindComponentByClass<UItemComponent>(); ItemComp && Inventory)
	{
		// 주운 행 (플러그인 ItemRow는 반사로 읽는다)
		const FStructProperty* RowProp = FindFProperty<FStructProperty>(ItemComp->GetClass(), TEXT("ItemRow"));
		const FName ItemRow = RowProp ? RowProp->ContainerPtrToValuePtr<FDataTableRowHandle>(ItemComp)->RowName : NAME_None;
		const bool bScorePickup = Target->ActorHasTag(USoulItemLibrary::ScorePickupTag);
		Inventory->CollectItem(Target);
		// 추가에 성공하면 플러그인이 액터를 지운다
		if (!IsValid(Target) || Target->IsActorBeingDestroyed())
		{
			OnItemCollected.Broadcast(Target);
			// 줍기 점수 (포션 행, 무기 첫 획득)
			if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this); Run && bScorePickup)
			{
				const USoulEquipmentComponent* EquipmentForScore = USoulEquipmentComponent::Find(GetOwner());
				const bool bWeapon = EquipmentForScore && EquipmentForScore->GetCatalog() && EquipmentForScore->GetCatalog()->FindWeaponByRow(ItemRow);
				Run->NotifyItemPickup(ItemRow, bWeapon);
			}
			// 무기 칸이 비었으면 바로 장착. 켜는 곳은 DA_EquipmentCatalog.bAutoEquipOnPickup (런 월드에서만 카탈로그가 있다)
			if (const USoulEquipmentComponent* Equipment = USoulEquipmentComponent::Find(GetOwner()))
			{
				Equipment->TryAutoEquipFromInventory(Inventory, ItemRow);
			}
		}
	}
}

const UInputAction* USoulInteractionComponent::GetInteractAction() const
{
	return SoulInteraction::ReadObjectProperty<UInputAction>(InventoryInput, TEXT("InventoryInteractAction"));
}

bool USoulInteractionComponent::DebugPressInteract()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	UEnhancedInputLocalPlayerSubsystem* Input = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	UInputAction* Action = SoulInteraction::ReadObjectProperty<UInputAction>(InventoryInput, TEXT("InventoryInteractAction"));
	if (!Input || !Action)
		return false;
	Input->InjectInputForAction(Action, FInputActionValue(true), {}, {});
	return true;
}

void USoulInteractionComponent::HandleTraceChanged(bool bHasTarget, FKey InputKey, AActor* TargetActor)
{
	TraceTarget = bHasTarget ? TargetActor : nullptr;
	if (!Inventory)
		return;

	const USoulInteractableComponent* Interactable = IsValid(TargetActor) ? TargetActor->FindComponentByClass<USoulInteractableComponent>() : nullptr;
	if (bHasTarget && Interactable)
	{
		// #40: 플러그인이 바라보던 상점 아이템(IInventoryInteractInterface)의 포커스를 먼저 끝낸다. 상점 아이템에서 런 오브젝트로 바로 넘어가는 경우
		ForwardTraceToInventory(false, InputKey, nullptr);
		// 팀 줍기 문구("F to Pickup ...") 대신 우리 문구 위젯 ("F 석상에 기도")
		if (Interactable->bEnabled)
		{
			ShowPrompt(InputKey, Interactable->PromptText);
		}
		else
		{
			HidePrompt();
		}
		return;
	}

	// 아이템이나 대상 없음은 플러그인 처리 그대로 (상점 아이템 포커스와 정보창 포함)
	HidePrompt();
	ForwardTraceToInventory(bHasTarget, InputKey, TargetActor);
}

void USoulInteractionComponent::ShowPrompt(FKey InputKey, const FText& Action)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC || !PC->IsLocalController())
		return;
	if (!PromptWidget)
	{
		PromptWidget = CreateWidget<USoulKeyHintWidget>(PC, PromptWidgetClass ? PromptWidgetClass : TSubclassOf<USoulKeyHintWidget>(USoulKeyHintWidget::StaticClass()));
		if (!PromptWidget)
			return;
		PromptWidget->SetFontSize(PromptFontSize);
		PromptWidget->AddToViewport(PromptZOrder);
		// SetPositionInViewport가 앵커를 되돌리므로 자리 다음에 앵커와 정렬
		PromptWidget->SetPositionInViewport(PromptOffset, false);
		PromptWidget->SetAnchorsInViewport(FAnchors(PromptAnchor.X, PromptAnchor.Y));
		PromptWidget->SetAlignmentInViewport(PromptAlignment);
	}
	FFormatNamedArguments Args;
	Args.Add(TEXT("Key"), InputKey.IsValid() ? InputKey.GetDisplayName(false) : INVTEXT("F"));
	Args.Add(TEXT("Action"), Action);
	PromptWidget->SetHint(FText::Format(PromptFormat, Args), PromptColor);
	PromptWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void USoulInteractionComponent::HidePrompt()
{
	if (PromptWidget)
	{
		PromptWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USoulInteractionComponent::ForwardTraceToInventory(bool bHasTarget, FKey InputKey, AActor* TargetActor)
{
	// protected라 이름으로 부른다
	if (UFunction* Function = Inventory ? Inventory->FindFunction(TEXT("HandleTraceItemChanged")) : nullptr)
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
