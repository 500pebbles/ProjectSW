// 

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Engine/EngineTypes.h"
#include "SoulAnimNotifyState_MeleeTrace.generated.h"

// 칼날 판정 구간. 시작 시 Events.Melee.TraceBegin, 매 Tick 두 소켓 사이를 구 스윕해서 맞은 액터마다 Events.Melee.Hit(Target = 맞은 액터)을 소유자에게 보냄.
// 노티파이 하나를 여러 캐릭터가 같이 쓰므로 상태를 저장하지 않음. 대상당 1번 처리는 받는 쪽(GA)에서.
UCLASS(meta = (DisplayName = "Soul Melee Trace"))
class PROJECTSW_API USoulAnimNotifyState_MeleeTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	USoulAnimNotifyState_MeleeTrace();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

protected:
	// 칼날 손잡이 쪽 소켓.
	UPROPERTY(EditAnywhere, Category = "Trace")
	FName StartSocketName = TEXT("BladeStart");

	// 칼끝 소켓.
	UPROPERTY(EditAnywhere, Category = "Trace")
	FName EndSocketName = TEXT("BladeEnd");

	// 칼날 두께. 놓치면 키움.
	UPROPERTY(EditAnywhere, Category = "Trace", meta = (ClampMin = "1.0"))
	float Radius = 20.f;

	// 판정할 오브젝트 타입. 기본 Pawn.
	UPROPERTY(EditAnywhere, Category = "Trace")
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
};