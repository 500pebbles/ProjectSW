// 
#include "Inventory/Player/InventoryInputComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Item/Interface/HighlightMaterialInterface.h"
#include "Item/ItemComponent.h"
#include "Item/Data/ItemData.h"

#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogInventoryInputComponent);



// Sets default values for this component's properties
UInventoryInputComponent::UInventoryInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	TraceLength = 600.0f;
	
	LastTracedItem = nullptr;
	CurrentTraceItem = nullptr;
	
	UE_LOG(LogTemp, Log, TEXT("InventoryInputComponent Ready"));
}

void UInventoryInputComponent::BeginPlay()
{
	Super::BeginPlay();
	
	APawn* PlayerPawn = Cast<APawn>(GetOwner());
	if (IsValid(PlayerPawn))
	{
		LocalGameController = Cast<APlayerController>(PlayerPawn->GetController());

		if (nullptr != LocalGameController)
		{
			// 플레이어 컨트롤러의 InputMappingContext에 인벤토리 매핑 컨텍스트 추가
			UEnhancedInputLocalPlayerSubsystem* Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalGameController->GetLocalPlayer());

			if (nullptr != Subsystem)
			{
				// 원래 인풋 액션이 맵핑되기까지 다음 입력 틱을 기다려야 하지만 버튼 텍스트 설정을 위해 강제로 즉시 리빌드 하도록 함
				FModifyContextOptions Options;
				Options.bForceImmediately = true;
				
				for (auto Context : IMC_Inventory)
				{
					Subsystem->AddMappingContext(Context, 0, Options);
				}
			}

			SetupInventoryInputComponent();
			SetupQuickSlotButtonText(Subsystem);
		}
	}
}

// Called every frame
void UInventoryInputComponent::TickComponent(float DeltaTime, ELevelTick TickType,
											 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	
	TraceItem();
}


void UInventoryInputComponent::SetupInventoryInputComponent()
{
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(LocalGameController->InputComponent);
	
	EnhancedInputComponent->BindAction(InventoryInteractAction, ETriggerEvent::Started, this, &UInventoryInputComponent::InventoryInteract);
	EnhancedInputComponent->BindAction(InventoryUIToggleAction, ETriggerEvent::Started, this, &UInventoryInputComponent::ToggleInventoryUI);
	EnhancedInputComponent->BindAction(UseQuickSlotItemAction, ETriggerEvent::Started, this, &UInventoryInputComponent::UseQuickSlotItem);
	EnhancedInputComponent->BindAction(SwitchPrevQuickSlotItemAction, ETriggerEvent::Started, this, &UInventoryInputComponent::SwitchQuickSlotItem, false);
	EnhancedInputComponent->BindAction(SwitchNextQuickSlotItemAction, ETriggerEvent::Started, this, &UInventoryInputComponent::SwitchQuickSlotItem, true);
}

void UInventoryInputComponent::SetupQuickSlotButtonText(UEnhancedInputLocalPlayerSubsystem* Subsystem)
{
	if (!IsValid(Subsystem))
	{
		return;
	}
	
	FKey UseKey;
	FKey SwitchPrevKey;
	FKey SwitchNextKey;
	
	TArray<FKey> TempKeyArray = Subsystem->QueryKeysMappedToAction(UseQuickSlotItemAction);
	if (!TempKeyArray.IsEmpty())
	{
		UseKey = TempKeyArray[0];	
	}
	
	TempKeyArray = Subsystem->QueryKeysMappedToAction(SwitchPrevQuickSlotItemAction);
	if (!TempKeyArray.IsEmpty())
	{
		SwitchPrevKey = TempKeyArray[0];
	}
	
	TempKeyArray = Subsystem->QueryKeysMappedToAction(SwitchNextQuickSlotItemAction);
	if (!TempKeyArray.IsEmpty())
	{
		SwitchNextKey = TempKeyArray[0];
	}
	
	OnSetupQuickSlotButtonText.Broadcast(UseKey, SwitchPrevKey, SwitchNextKey);
}

void UInventoryInputComponent::InventoryInteract()
{
	if (!CurrentTraceItem.IsValid())
	{
		return;
	}
	UE_LOG(LogInventoryInputComponent, Log, TEXT("Inventory Interact"));
	OnCollectItem.Broadcast(CurrentTraceItem.Get());
}

void UInventoryInputComponent::ToggleInventoryUI()
{
	UE_LOG(LogInventoryInputComponent, Log, TEXT("Toggle Inventory UI"));
	
	OnToggleInventoryUI.Broadcast();
}

void UInventoryInputComponent::UseQuickSlotItem()
{
	OnUseQuickSlotItem.Broadcast();	
}

void UInventoryInputComponent::SwitchQuickSlotItem(bool bIsHeadingNext)
{
	OnSwitchQuickSlotItem.Broadcast(bIsHeadingNext);
}

void UInventoryInputComponent::TraceItem()
{
	
	// 아이템 트레이스(충돌 검사를 통한 체크)
	// 유니티의 레이케스팅 처럼 월드 공간의 한 위치에서 시작해서 지정한 방향으로 가상의 선을 그림
	// 해당 선에 지정한 콜리전이 있으면 이를 추척하도록 함
	
	if (!IsValid(GEngine) || !IsValid(GEngine->GameViewport)) return;
	
	// 스크린의 중심 좌표 구하기(아래에서 스크린 좌표계를 월드 공간 좌표계로 변환하기 위해 사용)
	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	FVector2D ViewportCenter = ViewportSize / 2.0f;
	
	// 아이템 트레이스를 위한 선은 월드 공간 상에 그려짐
	// 따라서 Viewport 스크린 좌표계의 중심 위치를 월드 좌표계로 바꿔줄 필요가 있음
	// DeprojectScreenToWorld를 통해 화면 중심 위치를 지정한 월드 포지션 위치 및 방향인 월드 좌표계로 변환한다.
	// 해당 함수는 bool 타입으로 성공 여부를 반환하므로 값의 유효성 확인에도 사용할 수 있다.
	// 카메라가 바라보는 방향으로 콜리전 검사를 진행하기 위해 사용
	FVector TraceStart;
	FVector Forward;
	if(!UGameplayStatics::DeprojectScreenToWorld(
		LocalGameController, 
		ViewportCenter,
		TraceStart, 
		Forward)) return;
	
	const AActor* Owner = GetOwner();
	
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TraceItem), false, Owner);
	// 에디터에서 콘솔 명령을 통해 충돌 범위 디버깅을 위한 태그 조정 (명령어: TraceTag ItemTrace로 확인 가능)
	Params.TraceTag = TEXT("ItemTrace");
	
	// 트레이스 시작점 : 액터 위치
	FVector Start = Owner->GetActorLocation() + Owner->GetActorForwardVector();
	
	// 트레이스 끝점 : 액터 위치에서 카메라가 바라보는 방향으로 TraceLength만큼 떨어진 위치
	FVector End = TraceStart + Forward * TraceLength;
	
	FHitResult OutHitResult;
	
	// 충돌 결과를 가져오는 함수. 가장 가까운 충돌한 물체를 HitResult에 저장
	GetWorld()->SweepSingleByChannel(
		OutHitResult, Start, End, FQuat::Identity, ItemTraceChannel,
		FCollisionShape::MakeSphere(TraceRadius), Params);
	
	// 새 아이템을 Trace하기 시작함을 알리기 위해서 이전 Trace 액터와 현재 Trace액터를 저장하고 비교한다.
	LastTracedItem = CurrentTraceItem;
	CurrentTraceItem = OutHitResult.GetActor();
	
	// 현재 상호작용 가능한 액터를 검사하고 있다가 상호작용 대상 유무가 바뀌면 HUD에 알림
	const bool bHasTarget = CurrentTraceItem.IsValid();
	if (bHasTarget != bHasTraceTarget)
	{
		bHasTraceTarget = bHasTarget;
		
		if (InteractionKeys.IsEmpty())
		{
			UEnhancedInputLocalPlayerSubsystem* Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalGameController->GetLocalPlayer());
			InteractionKeys = Subsystem->QueryKeysMappedToAction(InventoryInteractAction);
		}

		// 타겟이 있으면 타겟도 전달 (이름 텍스트 적용 위함)
		OnTraceItemChanged.Broadcast(bHasTarget, InteractionKeys[0], CurrentTraceItem.Get());
	}
	
	if (CurrentTraceItem == LastTracedItem) return;
	
	// 새 아이템을 트레이싱 시작하면 이를 알린다 -> 이후 HUD에 UI로 안내를 띄우기 위해 사용할 예정
	if (CurrentTraceItem.IsValid())
	{
		// 임시 로그 처리
		UE_LOG(LogInventoryInputComponent, Log, TEXT("Tracing New Item"));

		// 어떤 아이템을 습득 가능한지 표시하기 위해 하이라이트 윤곽선 표시하기
		UActorComponent* HighlightMeshComponent = CurrentTraceItem->FindComponentByInterface(UHighlightMaterialInterface::StaticClass());
		if (IsValid(HighlightMeshComponent))
		{
			IHighlightMaterialInterface* HighlightMeshInterface = Cast<IHighlightMaterialInterface>(HighlightMeshComponent);
			if (nullptr != HighlightMeshInterface)
				HighlightMeshInterface->
			OnItemTraced();
		}
		
		// 이후 아이템에 접근해서 월드 상의 아이템 안보이게하거나 제거 후 인벤토리 데이터에 넣고
		// 인벤토리 UI에도 반영하도록 할 수 있도록 처리하기
		// 강의에서는 Item에 ItemComponent라는 것을 만들고 아이템 컴포넌트에 PickupMessage를 설정한
		// 후 그 값을 가져와서 PickupMessage에 Text를 할당하도록 처리함
	}
	
	// 트레이스를 중단해도 이를 알린다. -> HUD에 띄웠던 UI안내를 다시 내리기 위함
	if (LastTracedItem.IsValid())
	{
		// 임시 로그 처리
		UE_LOG(LogInventoryInputComponent, Log, TEXT("Stopped Tracing Item"));
		
		UActorComponent* HighlightMeshComponent = LastTracedItem->FindComponentByInterface(UHighlightMaterialInterface::StaticClass());
		if (IsValid(HighlightMeshComponent))
		{
			IHighlightMaterialInterface* HighlightMeshInterface = Cast<IHighlightMaterialInterface>(HighlightMeshComponent);
			if (nullptr != HighlightMeshInterface)
				HighlightMeshInterface->OffItemTraced();
		}
	}
}
