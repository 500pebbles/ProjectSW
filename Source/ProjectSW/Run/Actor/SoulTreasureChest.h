#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulTreasureChest.generated.h"

class APawn;
class UNiagaraSystem;
class USoundBase;
class UStaticMeshComponent;
class USoulInteractableComponent;
class USoulLootDropComponent;

/** 보물상자 (BP_TreasureChest, #11). Treasure 방에 하나. F로 1회 열고 드랍 표를 굴린다. 통메시라 뚜껑 대신 펄스와 FX */
UCLASS()
class PROJECTSW_API ASoulTreasureChest : public AActor
{
	GENERATED_BODY()

public:
	ASoulTreasureChest();

	/** Director가 층 점수를 넣는다 */
	void SetScoreValue(int32 InScore) { ScoreValue = InScore; }

	UFUNCTION(BlueprintCallable, Category = "Chest")
	void Open(APawn* User);

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Chest", meta = (DisplayName = "On Opened"))
	void K2_OnOpened(APawn* User);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest")
	TObjectPtr<USoulInteractableComponent> Interactable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest")
	TObjectPtr<USoulLootDropComponent> LootDrop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chest")
	TObjectPtr<UNiagaraSystem> OpenFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chest")
	TObjectPtr<USoundBase> OpenSound;

	/** 열 때 커졌다 돌아오는 크기와 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chest", meta = (ClampMin = "0"))
	float PulseScale = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chest", meta = (ClampMin = "0", Units = "s"))
	float PulseTime = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Chest")
	int32 ScoreValue = 0;

private:
	UFUNCTION()
	void HandleInteracted(APawn* User);

	void EndPulse();
	FVector BaseScale = FVector::OneVector;
	FTimerHandle PulseTimer;
};
