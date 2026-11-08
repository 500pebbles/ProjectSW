#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulRoomEventObject.generated.h"

class APawn;
class UNiagaraSystem;
class USceneComponent;
class USoundBase;
class UStaticMeshComponent;
class USoulInteractableComponent;
class UPointLightComponent;

UENUM(BlueprintType)
enum class ESoulRoomEventState : uint8
{
	Idle,
	Charging,
	Spent
};

/**
 * 이벤트룸 오브젝트 (#2). F → 점화 → 충전 → 효과 → 결과 → 입력 복구. 층당 1회.
 * 효과는 자식(회복, 함정, 순간이동)이 ApplyEvent에서 한다. 수치(Magnitude, 경험치, 점수)는 Director가 DA_DungeonFlow 행에서 넣는다.
 */
UCLASS(Abstract)
class PROJECTSW_API ASoulRoomEventObject : public AActor
{
	GENERATED_BODY()

public:
	ASoulRoomEventObject();

	/** Director가 스폰 직후 행 값을 넣는다 */
	void Configure(float InMagnitude, int32 InExpReward, int32 InScoreReward);

	UFUNCTION(BlueprintPure, Category = "Event")
	ESoulRoomEventState GetState() const { return State; }

	/** F 없이 바로 발동 (치트, 셀프 테스트) */
	UFUNCTION(BlueprintCallable, Category = "Event")
	void ForceActivate(APawn* User);

	bool IsSpent() const { return State == ESoulRoomEventState::Spent; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 효과. 자식이 덮는다 */
	UFUNCTION(BlueprintNativeEvent, Category = "Event")
	void ApplyEvent(APawn* User);

	/** 효과 뒤 BP에서 덧입힐 때 (독, 버프 등) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Event", meta = (DisplayName = "On Applied"))
	void K2_OnApplied(APawn* User);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Event")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Event")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Event")
	TObjectPtr<UStaticMeshComponent> TopMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Event")
	TObjectPtr<UPointLightComponent> Glow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Event")
	TObjectPtr<USoulInteractableComponent> Interactable;

	/** F 뒤 효과까지 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event", meta = (ClampMin = "0", Units = "s"))
	float ChargeTime = 0.f;

	/** 효과 뒤 입력을 돌려주기까지 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event", meta = (ClampMin = "0", Units = "s"))
	float RecoverDelay = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event|FX")
	TObjectPtr<UNiagaraSystem> IgniteFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event|FX")
	TObjectPtr<USoundBase> IgniteSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event|FX")
	TObjectPtr<UNiagaraSystem> ResultFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event|FX")
	TObjectPtr<USoundBase> ResultSound;

	/** 행 값 (런타임) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Event")
	float Magnitude = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Event")
	int32 ExpReward = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Event")
	int32 ScoreReward = 0;

private:
	UFUNCTION()
	void HandleInteracted(APawn* User);

	void FinishCharge();
	void Recover();
	void LockUser(APawn* User);
	void UnlockUser();
	bool bUserLocked = false;

	ESoulRoomEventState State = ESoulRoomEventState::Idle;
	TWeakObjectPtr<APawn> ActiveInstigator;
	FTimerHandle ChargeTimer;
	FTimerHandle RecoverTimer;
};
