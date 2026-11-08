#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulFloorPortal.generated.h"

class USphereComponent;

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

protected:
	virtual void BeginPlay() override;

	/** 플레이어가 들어오는 범위. 반경은 BP_FloorPortal의 컴포넌트에서 고친다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	TObjectPtr<USphereComponent> Trigger;

	/** 켜져 있으면 플레이어가 닿는 순간 들어간다 (단계 B 임시. 단계 D에서 끄고 F로 바꾼다) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Portal")
	bool bEnterOnOverlap = false;

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
