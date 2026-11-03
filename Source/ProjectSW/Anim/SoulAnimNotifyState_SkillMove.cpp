#include "Anim/SoulAnimNotifyState_SkillMove.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

namespace
{
	const FName SkillMoveSourceName = TEXT("SoulSkillMove");
}

void USoulAnimNotifyState_SkillMove::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MoveDistance <= 0.0f || TotalDuration <= KINDA_SMALL_NUMBER || !MeshComp)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner());
	if (!Character)
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	// 이전 이동 소스가 남아있다면 제거
	MoveComp->RemoveRootMotionSource(SkillMoveSourceName);

	// 노티파이 막대 길이(TotalDuration) 동안 MoveDistance만큼 이동하도록 속도(Strength) 자동 계산
	const float Strength = MoveDistance / TotalDuration;

	TSharedPtr<FRootMotionSource_ConstantForce> ConstantForce = MakeShared<FRootMotionSource_ConstantForce>();
	ConstantForce->InstanceName = SkillMoveSourceName;
	ConstantForce->AccumulateMode = ERootMotionAccumulateMode::Override;
	ConstantForce->Priority = 5;
	ConstantForce->Force = Character->GetActorForwardVector() * Strength;
	ConstantForce->Duration = TotalDuration;
	ConstantForce->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	ConstantForce->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
	ConstantForce->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);

	MoveComp->ApplyRootMotionSource(ConstantForce);
}

void USoulAnimNotifyState_SkillMove::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner()))
	{
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			// 구간이 끝나거나 대시/스킬 캔슬로 몽타주가 끊기면 즉시 전진 중단
			MoveComp->RemoveRootMotionSource(SkillMoveSourceName);
		}
	}
}