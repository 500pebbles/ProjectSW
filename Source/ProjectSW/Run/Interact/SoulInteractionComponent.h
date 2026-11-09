#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "SoulInteractionComponent.generated.h"

class AController;
class APawn;
class UInventoryComponent;
class UInventoryInputComponent;
class UInputAction;
class USoulKeyHintWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoulItemCollectedSignature, AActor*, ItemActor);

/**
 * 폰의 F 라우터 (팀 폰 BP_WorkerPlayerCharacter에 붙는다, D8. 런 월드도 같은 폰을 쓴다).
 * 무기 자동 장착은 DA_EquipmentCatalog.bAutoEquipOnPickup이 켜고, 카탈로그가 없는 팀 맵에서는 꺼진다.
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

	/** 아이템 트레이스가 지금 잡은 대상 */
	AActor* GetTraceTarget() const { return TraceTarget.Get(); }

	/** 팀 인벤토리 입력의 상호작용(F) 액션. 키 안내 문구용 */
	const UInputAction* GetInteractAction() const;

	/** 디버그: F 입력 액션을 주입한다 (실제 트레이스와 F 경로를 그대로 탄다) */
	bool DebugPressInteract();

	/** 아이템을 주웠을 때 (무기 자동 장착이 여기에 붙는다) */
	UPROPERTY(BlueprintAssignable, Category = "Interact")
	FSoulItemCollectedSignature OnItemCollected;

protected:
	virtual void BeginPlay() override;

	/** 인벤토리 컴포넌트 (BP_SoulInventoryComponent). 비어 있으면 인벤토리 없이 상호작용만 안 된다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interact")
	TSubclassOf<UInventoryComponent> InventoryClass;

	/** 인벤토리 입력 컴포넌트 (BP_SoulInventoryInputComponent). F, I, E, Q, R, C와 아이템 트레이스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interact")
	TSubclassOf<UInventoryInputComponent> InventoryInputClass;

	/**
	 * 런 상호작용 물체(석상, 게이트, 이벤트, 상자, 포탈, 명예의 전당)의 문구 위젯. 팀 줍기 문구는 "to Pickup"이 붙어 쓰지 않는다.
	 * 비우면 코드 기본 위젯 USoulKeyHintWidget. 팀 아이템 줍기 문구는 그대로 팀 HUD가 띄운다
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interact|Prompt")
	TSubclassOf<USoulKeyHintWidget> PromptWidgetClass;

	/** {Key}는 상호작용 키, {Action}은 물체의 PromptText */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interact|Prompt")
	FText PromptFormat = NSLOCTEXT("SoulRun", "InteractPrompt", "{Key}  {Action}");

	/** 화면 기준 앵커 (0.5, 0.5 = 가운데) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interact|Prompt")
	FVector2D PromptAnchor = FVector2D(0.5f, 0.5f);

	/** 앵커 기준 위젯 정렬 (0, 0.5 = 왼쪽 가운데가 앵커에) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interact|Prompt")
	FVector2D PromptAlignment = FVector2D(0.f, 0.5f);

	/** 앵커에서 떨어진 자리 (px) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interact|Prompt")
	FVector2D PromptOffset = FVector2D(50.f, 0.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interact|Prompt", meta = (ClampMin = "8"))
	int32 PromptFontSize = 20;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interact|Prompt")
	FLinearColor PromptColor = FLinearColor(1.f, 0.95f, 0.85f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interact|Prompt")
	int32 PromptZOrder = 15;

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	UFUNCTION()
	void HandleCollect(AActor* Target);

	UFUNCTION()
	void HandleTraceChanged(bool bHasTarget, FKey InputKey, AActor* TargetActor);
	void ForwardTraceToInventory(bool bHasTarget, FKey InputKey, AActor* TargetActor);
	void ShowPrompt(FKey InputKey, const FText& Action);
	void HidePrompt();

	UPROPERTY(Transient)
	TObjectPtr<USoulKeyHintWidget> PromptWidget;

	UFUNCTION()
	void HandleEquipmentToggled();

	void InitializeInventory();

	UPROPERTY(Transient)
	TObjectPtr<UInventoryComponent> Inventory;

	UPROPERTY(Transient)
	TObjectPtr<UInventoryInputComponent> InventoryInput;

	bool bEquipmentOpen = false;
	bool bInitScheduled = false;
	TWeakObjectPtr<AActor> TraceTarget;
};
