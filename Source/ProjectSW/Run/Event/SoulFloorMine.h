#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "SoulFloorMine.generated.h"

class UGameplayEffect;
class UNiagaraSystem;
class UPointLightComponent;
class USoundBase;
class USphereComponent;
class UStaticMeshComponent;

/**
 * 바닥 지뢰 (BP_FloorMine, D22). 플레이어가 밟으면 경고 → FuseTime 뒤 폭발. 폭발 반경 안이고 무적이 아니면 최대 체력 비율 피해(기본 비치명).
 * 몹은 밟아도 안 터지고 피해도 없다. 1회용. 위치와 개수는 Director가 DA_DungeonFlow.MineRows와 DA_MinePatterns로 정한다.
 */
UCLASS()
class PROJECTSW_API ASoulFloorMine : public AActor
{
	GENERATED_BODY()

public:
	ASoulFloorMine();

	/** Director가 층 행의 피해 비율을 넣는다 */
	void SetDamageRatio(float InRatio) { DamageRatio = InRatio; }

	/** 디버그: 밟은 것처럼 발동 */
	void ForceTrigger();

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Mine", meta = (DisplayName = "On Armed"))
	void K2_OnArmed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Mine", meta = (DisplayName = "On Detonated"))
	void K2_OnDetonated();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mine")
	TObjectPtr<USphereComponent> Sphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mine")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mine")
	TObjectPtr<UPointLightComponent> WarnLight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine", meta = (ClampMin = "0", Units = "s"))
	float FuseTime = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine", meta = (ClampMin = "0", Units = "cm"))
	float BlastRadius = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine")
	bool bCanKill = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine")
	FGameplayTag MagnitudeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine|FX", meta = (ClampMin = "0", Units = "s"))
	float BlinkInterval = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine|FX", meta = (ClampMin = "0"))
	float BlinkIntensity = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine|FX")
	TObjectPtr<UNiagaraSystem> WarnFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine|FX")
	TObjectPtr<USoundBase> WarnSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine|FX")
	TObjectPtr<UNiagaraSystem> BlastFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine|FX")
	TObjectPtr<USoundBase> BlastSound;

	/** 층 행 값 (런타임) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Mine")
	float DamageRatio = 0.f;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void Arm();
	void Blink();
	void Detonate();

	bool bArmed = false;
	bool bBlinkOn = false;
	float BaseIntensity = 0.f;
	FTimerHandle FuseTimer;
	FTimerHandle BlinkTimer;
};
