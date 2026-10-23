// 
#include "Player/InventoryInputComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Interface/HighlightMaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Player/InventoryPlayerController.h"


// Sets default values for this component's properties
UInventoryInputComponent::UInventoryInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	TraceLength = 600.0f;
	
	LastTracedItem = nullptr;
	CurrentTraceItem = nullptr;
	
	// 월드 세팅의 플레이어 컨트롤러 갖고 오기
	GamePlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
}

void UInventoryInputComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (nullptr != GamePlayerController)
	{
		// 플레이어 컨트롤러의 InputMappintContext에 생성한 인벤토리 맵핑 컨텍스트 추가하기
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GamePlayerController->GetLocalPlayer());
		
		if (nullptr != Subsystem)
		{
			for (auto Context : IMC_Inventory)
			{
				Subsystem->AddMappingContext(Context, 0);
			}
		}
	}
	
	CreateInventoryHUDWidget();
}

void UInventoryInputComponent::SetupInventoryInputComponent()
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(GamePlayerController->InputComponent);
	
	EnhancedInputComponent->BindAction(InventoryInteractAction, ETriggerEvent::Started, this, &UInventoryInputComponent::InventoryInteract);
}

void UInventoryInputComponent::InventoryInteract()
{
	UE_LOG(LogInventorySystem, Log, TEXT("Inventory Interact"));
}

void UInventoryInputComponent::CreateInventoryHUDWidget()
{
	InventoryHUDWidget = CreateWidget<UInventoryHUDWidget>(GamePlayerController, InventoryHUDWidgetClass);
	
	if (IsValid(InventoryHUDWidget))
	{
		InventoryHUDWidget->AddToViewport();
	}
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
	FVector TraceStart;
	FVector Forward;
	if(!UGameplayStatics::DeprojectScreenToWorld(
		this, 
		ViewportCenter, 
		TraceStart, 
		Forward)) return;
	
	// 트레이스를 위한 가상의 범위(선)을 그릴 때 TraceStart부터 TraceEnd까지 그림
	// Forward는 방향만 갖는 방향벡터이기 때문에 길이 값은 TraceLength를 곱해 지정 가능하다.
	const FVector TraceEnd = TraceStart + Forward * TraceLength;
	
	// 트레이스 결과를 담아서 저장하기
	FHitResult HitResult;
	
	// LineTraceSingleByChannel: 월드 좌표계의 시작 위치와 끝 위치까지 선을 그려서 해당 선과 파라미터로
	// 전달한 콜리전 프로파일로 지정한 트레이스 채널에 해당하는 collision이 있다면 최초 collision에 해당하는
	// 충돌 결과를 반환한다.
	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ItemTraceChannel);
	
	// 새 아이템을 Trace하기 시작함을 알리기 위해서 이전 Trace 액터와 현재 Trace액터를 저장하고 비교한다.
	LastTracedItem = CurrentTraceItem;
	CurrentTraceItem = HitResult.GetActor();
	
	// 액터가 유효하지 않으면 픽업 메시지 숨김 처리
	if (!CurrentTraceItem.IsValid())
	{
		if (IsValid(InventoryHUDWidget)) InventoryHUDWidget->HideInteractionMessage();
	}
	
	if (CurrentTraceItem == LastTracedItem) return;
	
	// 새 아이템을 트레이싱 시작하면 이를 알린다 -> 이후 HUD에 UI로 안내를 띄우기 위해 사용할 예정
	if (CurrentTraceItem.IsValid())
	{
		// 임시 로그 처리
		UE_LOG(LogInventorySystem, Log, TEXT("Tracing New Item"));
		
		if (IsValid(InventoryHUDWidget)) InventoryHUDWidget->ShowInteractionMessage();
		
		// 어떤 아이템을 습득 가능한지 표시하기 위해 하이라이트 윤곽선 표시하기
		UActorComponent* HighlightMeshComponent = CurrentTraceItem->FindComponentByInterface(UHighlightMaterialInterface::StaticClass());
		if (IsValid(HighlightMeshComponent))
		{
			IHighlightMaterialInterface* HighlightMeshInterface = Cast<IHighlightMaterialInterface>(HighlightMeshComponent);
			if (nullptr != HighlightMeshInterface)
				HighlightMeshInterface->OnItemTraced();
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
		UE_LOG(LogInventorySystem, Log, TEXT("Stopped Tracing Item"));
		
		UActorComponent* HighlightMeshComponent = LastTracedItem->FindComponentByInterface(UHighlightMaterialInterface::StaticClass());
		if (IsValid(HighlightMeshComponent))
		{
			IHighlightMaterialInterface* HighlightMeshInterface = Cast<IHighlightMaterialInterface>(HighlightMeshComponent);
			if (nullptr != HighlightMeshInterface)
				HighlightMeshInterface->OffItemTraced();
		}
	}
}

// Called every frame
void UInventoryInputComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

