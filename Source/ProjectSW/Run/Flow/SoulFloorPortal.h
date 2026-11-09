#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulFloorPortal.generated.h"

class USphereComponent;
class USoulInteractableComponent;
class USoulHighlightComponent;
class UWidgetComponent;
class APawn;

/**
 * 다음 층으로 내려가는 포탈 (BP_FloorPortal). Director가 Exit 방 전투가 끝난 뒤 스폰하고 층을 바꿀 때 지운다.
 * 들어가면 Director::RequestDescend. 단계 B는 bEnterOnOverlap(밟으면 바로)이고, 단계 D에서 F 상호작용으로 바꾼다.
 * 외형(FX, 소리)은 BP와 단계 F의 DA_Presentation에서 붙인다.
 */
UCLASS()
class PROJECTSW_API ASoulFloorPortal : public AActor
{
	GENERATED_BODY()

public:
	ASoulFloorPortal();

	/** 포탈에 들어간다 (층 이동 요청). 오버랩과 이후 F 상호작용이 이 함수를 부른다 */
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void EnterPortal();

	USphereComponent* GetTrigger() const { return Trigger; }

	/** 멀리서도 보이는 화면 표시 (벽에 가리지 않는다). Director가 포탈을 세울 때 DA 문구로 켠다 */
	void ShowMarker(const FText& InText, const FLinearColor& InColor);

protected:
	virtual void BeginPlay() override;

	/** 플레이어가 들어오는 범위. 반경은 BP_FloorPortal의 컴포넌트에서 고친다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	TObjectPtr<USphereComponent> Trigger;

	/** F로 들어간다 (프롬프트, 반경은 BP) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	TObjectPtr<USoulInteractableComponent> Interactable;

	/** 하이라이트 (색과 세기는 BP) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	TObjectPtr<USoulHighlightComponent> Highlight;

	/** 화면 공간 표시 (USoulKeyHintWidget). 높이는 BP */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	TObjectPtr<UWidgetComponent> Marker;

	/** 켜져 있으면 플레이어가 닿는 순간 들어간다 (단계 B 임시. 지금은 끄고 F로 들어간다) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Portal")
	bool bEnterOnOverlap = false;

private:
	UFUNCTION()
	void HandleInteracted(APawn* User);

	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
