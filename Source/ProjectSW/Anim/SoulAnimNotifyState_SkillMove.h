#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SoulAnimNotifyState_SkillMove.generated.h"

// 몽타주의 지정된 구간 동안 캐릭터를 정면으로 MoveDistance(cm)만큼 전진시키는 노티파이
UCLASS(meta = (DisplayName = "Soul Skill Move"))
class PROJECTSW_API USoulAnimNotifyState_SkillMove : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	// 이 노티파이 구간 동안 전진할 총 거리 (cm 단위, 예: 250.0 = 2.5m 전진)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float MoveDistance = 250.0f;
};