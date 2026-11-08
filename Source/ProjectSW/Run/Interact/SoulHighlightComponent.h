#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/Interface/HighlightMaterialInterface.h"
#include "SoulHighlightComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMeshComponent;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * 상호작용 오브젝트 하이라이트 (이벤트 3종, 포탈, 상자).
 * 평소에는 멀리서도 보이는 약한 프레넬 오버레이와 빛이 펄스하고, 트레이스에 잡히거나(팀 플러그인 IHighlightMaterialInterface) 가까이 가면 세진다.
 * 주인의 USoulInteractableComponent가 꺼지면(1회용 사용, Spent) 스스로 꺼진다. 색과 세기, 펄스는 BP 칸에서 고친다.
 * 오버레이 재질 파라미터: Color(벡터), Intensity(스칼라), FresnelExponent(스칼라)
 */
UCLASS(ClassGroup = (Soul), meta = (BlueprintSpawnableComponent))
class PROJECTSW_API USoulHighlightComponent : public UActorComponent, public IHighlightMaterialInterface
{
	GENERATED_BODY()

public:
	USoulHighlightComponent();

	/** 켜고 끈다. 끄면 오버레이, 빛, 빛기둥이 모두 사라진다 */
	UFUNCTION(BlueprintCallable, Category = "Highlight")
	void SetHighlightActive(bool bInActive);

	UFUNCTION(BlueprintPure, Category = "Highlight")
	bool IsHighlightActive() const { return bActive; }

	/** 트레이스에 잡혔거나 가까이 있어 강하게 빛나는 중 */
	UFUNCTION(BlueprintPure, Category = "Highlight")
	bool IsFocused() const { return bTraced || bNear; }

	// IHighlightMaterialInterface (팀 플러그인 아이템 트레이스가 부른다)
	virtual void OnItemTraced() override;
	virtual void OffItemTraced() override;

	/** 색 (회복 초록, 순간이동 파랑, 함정 붉은빛, 포탈 보라, 상자 금색) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	FLinearColor Color = FLinearColor(1.f, 0.85f, 0.4f);

	/** 메시에 덧씌울 오버레이 재질 (M_RunHighlight). 비우면 빛만 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	TObjectPtr<UMaterialInterface> OverlayMaterial;

	/** 평소 오버레이 세기 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight", meta = (ClampMin = "0"))
	float IdleIntensity = 1.5f;

	/** 트레이스, 가까움일 때 오버레이 세기 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight", meta = (ClampMin = "0"))
	float FocusIntensity = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight", meta = (ClampMin = "0.1"))
	float FresnelExponent = 3.f;

	/** 펄스 빠르기 (초당 번) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight", meta = (ClampMin = "0", Units = "Hz"))
	float PulseSpeed = 0.6f;

	/** 펄스 깊이 (0이면 펄스 없음, 0.4면 세기의 ±40%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight", meta = (ClampMin = "0", ClampMax = "1"))
	float PulseDepth = 0.35f;

	/** 이 거리 안이면 강하게 (0이면 트레이스만) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight", meta = (ClampMin = "0", Units = "cm"))
	float NearRadius = 450.f;

	/** 평소에서 강하게로 넘어가는 빠르기 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight", meta = (ClampMin = "0"))
	float BlendSpeed = 6.f;

	/** 평소 빛 세기 (0이면 빛 없음). 강할 때는 FocusLightScale배 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Light", meta = (ClampMin = "0"))
	float LightIntensity = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Light", meta = (ClampMin = "1"))
	float FocusLightScale = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Light", meta = (ClampMin = "0", Units = "cm"))
	float LightRadius = 500.f;

	/** 주인 기준 빛 자리 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Light")
	FVector LightOffset = FVector(0.f, 0.f, 120.f);

	/** 멀리서 보이는 빛기둥 (포탈) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Beacon")
	bool bBeacon = false;

	/** 빛기둥 메시 (비우면 엔진 원기둥) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Beacon", meta = (EditCondition = "bBeacon"))
	TObjectPtr<UStaticMesh> BeaconMesh;

	/** 빛기둥 크기 (엔진 원기둥은 지름 100, 높이 100) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Beacon", meta = (EditCondition = "bBeacon"))
	FVector BeaconScale = FVector(0.8f, 0.8f, 20.f);

	/** 빛기둥 오버레이 세기 (평소 세기에 곱한다) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Beacon", meta = (EditCondition = "bBeacon", ClampMin = "0"))
	float BeaconIntensityScale = 0.6f;

	/** 빛기둥 프레넬 (작을수록 기둥 안쪽도 찬다) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Beacon", meta = (EditCondition = "bBeacon", ClampMin = "0.1"))
	float BeaconFresnelExponent = 1.5f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void ApplyOverlay(bool bOn);
	bool IsOwnerInteractable() const;
	float UpdateNear();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BeaconMID;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Beacon;

	TArray<TWeakObjectPtr<UMeshComponent>> Meshes;
	bool bActive = true;
	bool bTraced = false;
	bool bNear = false;
	float Focus = 0.f;
	float NearCheckTimer = 0.f;
};
