// 

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Interface/InventoryInputInterface.h"
#include "Runtime/Engine/Classes/Components/ActorComponent.h"
#include "InventoryInputComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogInventoryInputComponent, Log, All);

class UInputMappingContext;
class UInputAction;

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UInventoryInputComponent : public UActorComponent, public IInventoryInputInterface
{
	GENERATED_BODY()
	
public:
	FOnToggleInventoryUIDelegate OnToggleInventoryUI;
	FOnCollectItemDelegate OnCollectItem;
	FOnUseQuickSlotItemDelegate OnUseQuickSlotItem;
	FOnTraceItemChangedDelegate OnTraceItemChanged;
	FOnSwitchQuickSlotItemDelegate OnSwitchQuickSlotItem;

public:
	// Sets default values for this component's properties
	UInventoryInputComponent();
	
public:
	virtual void BeginPlay() override;
	
	// 이 함수를 플레이어 컨트롤러의 SetupInputComponent()에서 호출할 것
	void SetupInventoryInputComponent();

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
	
public:
	virtual FOnToggleInventoryUIDelegate& GetOnToggleInventoryUI() override {return OnToggleInventoryUI;}
	virtual FOnCollectItemDelegate& GetOnCollectItem() override {return OnCollectItem;};
	virtual FOnUseQuickSlotItemDelegate& GetOnUseQuickSlotItem() override {return OnUseQuickSlotItem;}
	virtual FOnTraceItemChangedDelegate& GetOnTraceItemChanged() override {return OnTraceItemChanged;}
	virtual FOnSwitchQuickSlotItemDelegate& GetSwitchQuickSlotItem() override {return OnSwitchQuickSlotItem;}
	
	// 입력 관련 함수
private:
	void InventoryInteract();
	
	void ToggleInventoryUI();
	
	void UseQuickSlotItem();
	
	void SwitchQuickSlotItem(bool bIsHeadingNext);
	
private:
	// 아이템을 바라보고 있을 때 범위 내에 있다면 상호작용이 가능함을 표시하기 위한 함수
	// 해당 함수는 아이템 획득을 진행할 때 획득을 위한 상호작용 가능 범위를 영역이 아닌 바라보고 있는
	// 방향에서 일정 거리 내에 있는지로 판단하는 것을 전재로 한다.
	void TraceItem();
	
private:
	UPROPERTY()
	TObjectPtr<APlayerController> LocalGameController;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TArray<TObjectPtr<UInputMappingContext>> IMC_Inventory;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UInputAction> InventoryInteractAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UInputAction> InventoryUIToggleAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UInputAction> UseQuickSlotItemAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UInputAction> SwitchNextQuickSlotItemAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UInputAction> SwitchPrevQuickSlotItemAction;
	
	// 아이템 상호작용 검사 섹션
private:
	// 에디터에서 설정할 아이템 감지를 위한 유효 범위(거리)
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	float TraceLength;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	float TraceRadius;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	FVector TraceOffset;
	
	// 아이템 트레이스에 사용할 콜리전 채널
	// EditDefaultsOnly로 에디터에서 설정하기 위해서는 Enum타입으로 되어야하는데, ECollisionChannel은
	// 자체적으로 변환이 불가능하기 때문에 형변환이 필요함
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TEnumAsByte<ECollisionChannel> ItemTraceChannel;
	
	// 아래 변수는 트레이싱하는 아이템이 변경됬을 때 이를 판정하기 위한 변수이기 때문에 계속 갖고 있어야하므로
	// GC에서 회수하는 것을 방지하기 위해서 UPROPERTY를 사용한다. UPROPERTY를 붙이면 메모리 관리를 위해
	// 언리얼의 스마트 포인터인 TObjectPtr를 붙여야 하는데, 일반 TObjectPtr은 SharedPtr처럼 원본 오브젝트가
	// 더이상 필요가 없어져 파괴했을 때도 다른 참조가 있다면
	// 메모리에 계속 갖고 있기 떄문에 액터 제거 후에도 GC에서 회수해가지 않는다.
	// 때문에 TWeakObjectPtr로 언제든지 파괴되어 메모리에서 해제될 수 있는 상태로 두고 액터에 안전하게 접근하기 위해
	// TWeakObjectPtr을 사용한다.
	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentTraceItem;
	
	UPROPERTY()
	TWeakObjectPtr<AActor> LastTracedItem;
	
	bool bHasTraceTarget = false;
	
	// 아이템 상호작용 메시지에서 플레이어의 상호작용 키를 표시하기 위해 저장하는 변수
	UPROPERTY()
	TArray<FKey> InteractionKeys;
};
