#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "SoulInteractionComponent.generated.h"

class AController;
class APawn;
class UInventoryComponent;
class UInventoryInputComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoulItemCollectedSignature, AActor*, ItemActor);

/**
 * 폰의 F 라우터 (BP_WorkerPlayerCharacter에 붙어 런 폰이 상속, D8).
 * 컨트롤러가 생긴 다음 틱에 인벤토리 플러그인 컴포넌트 2개를 런타임 등록하고(L_Dungeon과 L_PMG의 지연 스폰 대응),
 * F 대상이 USoulInteractableComponent면 Interact, 아이템이면 인벤토리 CollectItem으로 나눈다. 플러그인 파일은 고치지 않는다.
 */
UCLASS(ClassGroup = (Soul), meta = (BlueprintSpawnableComponent))
class PROJECTSW_API USoulInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USoulInteractionComponent();

	static USoulInteractionComponent* Find(const AActor* Actor);

	/** 열린 인벤토리와 장비창을 닫는다 (층 전환, 사망, 맵 이동 직전) */
	UFUNCTION(BlueprintCallable, Category = "Interact")
	void CloseInventoryUI();

	UFUNCTION(BlueprintPure, Category = "Interact")
	UInventoryComponent* GetInventory() const { return Inventory; }

	/** F를 누른 것과 같은 경로로 대상을 처리한다 (상호작용 물체면 Interact, 아이템이면 줍기) */
	void CollectTarget(AActor* Target) { HandleCollect(Target); }

	/** 아이템을 주웠을 때 (무기 자동 장착이 여기에 붙는다) */
	UPROPERTY(BlueprintAssignable, Category = "Interact")
	FSoulItemCollectedSignature OnItemCollected;

	/** 켜면 무기를 주울 때 무기 칸이 비어 있으면 바로 장착한다 (BP_RunPlayerCharacter에서만 켠다) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interact")
	bool bAutoEquipOnPickup = false;

protected:
	virtual void BeginPlay() override;

	/** 인벤토리 컴포넌트 (BP_SoulInventoryComponent). 비어 있으면 인벤토리 없이 상호작용만 안 된다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interact")
	TSubclassOf<UInventoryComponent> InventoryClass;

	/** 인벤토리 입력 컴포넌트 (BP_SoulInventoryInputComponent). F, I, E, Q, R, C와 아이템 트레이스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interact")
	TSubclassOf<UInventoryInputComponent> InventoryInputClass;

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	UFUNCTION()
	void HandleCollect(AActor* Target);

	UFUNCTION()
	void HandleTraceChanged(bool bHasTarget, FKey InputKey, AActor* TargetActor);

	UFUNCTION()
	void HandleEquipmentToggled();

	void InitializeInventory();

	UPROPERTY(Transient)
	TObjectPtr<UInventoryComponent> Inventory;

	UPROPERTY(Transient)
	TObjectPtr<UInventoryInputComponent> InventoryInput;

	bool bEquipmentOpen = false;
	bool bInitScheduled = false;
};
