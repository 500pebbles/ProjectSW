// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InventoryPlayerController.generated.h"

class UInventoryHUDWidget;
class UInputAction;
class UInputMappingContext;

DECLARE_LOG_CATEGORY_EXTERN(LogInventorySystem, Log, All);

/**
 * This player controller is just for input test only
 */
UCLASS()
class INVENTORYSYSTEM_API AInventoryPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AInventoryPlayerController();
	
protected:
	virtual void BeginPlay() override;
	
	virtual void SetupInputComponent() override;
	
	virtual void Tick(float DeltaSeconds) override;
	
public:
	void InventoryInteract();
	
	void CreateInventoryHUDWidget();
	
	// 아이템을 바라보고 있을 때 범위 내에 있다면 상호작용이 가능함을 표시하기 위한 함수
	// 해당 함수는 아이템 획득을 진행할 때 획득을 위한 상호작용 가능 범위를 영역이 아닌 바라보고 있는
	// 방향에서 일정 거리 내에 있는지로 판단하는 것을 전재로 한다.
	void TraceItem();
	
private:
	// EditDefaultsOnly parameter requires to initialize this variable in BluePrintClass manually
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TArray<TObjectPtr<UInputMappingContext>> IMC_Inventory;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UInputAction> InventoryInteractAction;
	
	// HUD 섹션
private:
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UInventoryHUDWidget> InventoryHUDWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UInventoryHUDWidget> InventoryHUDWidget;
	
	// 에디터에서 설정할 아이템 감지를 위한 유효 범위(거리)
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	float TraceLength;
	
	// 아이템 트레이스에 사용할 콜리전 채널
	// EditDefaultsOnly로 에디터에서 설정하기 위해서는 Enum타입으로 되어야하는데, ECollisionChannel은
	// 자체적으로 변환이 불가능하기 때문에 형변환이 필요함
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TEnumAsByte<ECollisionChannel> ItemTraceChannel;
	
	// 아래 변수는 트레이싱하는 아이템이 변경됬을 때 이를 판정하기 위한 변수이기 때문에 계속 갖고 있어야하므로
	// GC에서 회수하는 것을 방지하기 위해서 UPROPERTY를 사용한다. UPROPERTY를 붙이면 메모리 관리를 위해
	// 언리얼의 스마트 포인터인 TObjectPtr를 붙여야 하는데, 일반 TObjectPtr은 SharedPtr처럼 원본 오브젝트가
	// 더이상 필요가 없어져 파괴했을 때도 메모리에 계속 갖고 있기 떄문에 액터 제거 후에도 GC에서 회수해가지 않는다.
	// 때문에 TWeakObjectPtr로 언제든지 파괴되어 메모리에서 해제될 수 있는 상태로 두고 액터에 안전하게 접근하기 위해
	// TWeakObjectPtr을 사용한다.
	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentTraceItem;
	
	UPROPERTY()
	TWeakObjectPtr<AActor> LastTracedItem;

};
