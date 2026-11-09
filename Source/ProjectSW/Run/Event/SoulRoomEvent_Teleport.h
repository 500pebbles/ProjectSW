#pragma once

#include "CoreMinimal.h"
#include "Run/Event/SoulRoomEventObject.h"
#include "SoulRoomEvent_Teleport.generated.h"

/**
 * 순간이동 이벤트 (BP_RoomEvent_Teleport). 연결된 방 중 현재와 출구를 빼고 가중치로 골라, 그 방 임의 문 안쪽으로 옮긴다.
 * 가중치: 미방문 전투방 / 미방문 비전투방 / 방문한 방 / 입구. 미방문 전투방이면 트리거 안이라 매복 전투가 기존 코드로 시작된다.
 */
UCLASS()
class PROJECTSW_API ASoulRoomEvent_Teleport : public ASoulRoomEventObject
{
	GENERATED_BODY()

protected:
	virtual void ApplyEvent_Implementation(APawn* User) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport", meta = (ClampMin = "0"))
	float WeightUnvisitedCombat = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport", meta = (ClampMin = "0"))
	float WeightUnvisitedPeaceful = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport", meta = (ClampMin = "0"))
	float WeightVisited = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport", meta = (ClampMin = "0"))
	float WeightEntrance = 0.f;

	/** 문 안쪽으로 들어온 거리 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport", meta = (ClampMin = "0", Units = "cm"))
	float LandingDepth = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport", meta = (ClampMin = "0", Units = "s"))
	float FadeTime = 0.f;

	/** 착지 높이 여유 (바닥 위) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport", meta = (ClampMin = "0", Units = "cm"))
	float LandingLift = 0.f;

	/** 착지 뒤 무적 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport", meta = (ClampMin = "0", Units = "s"))
	float LandingInvincibleTime = 0.f;

private:
	void DoTeleport(TWeakObjectPtr<APawn> WeakPawn, FVector Location);
	FTimerHandle FadeTimer;
	FTimerHandle InvincibleTimer;
};
