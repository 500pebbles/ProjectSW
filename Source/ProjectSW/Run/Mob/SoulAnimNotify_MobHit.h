#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "SoulAnimNotify_MobHit.generated.h"

class ASoulMobProjectile;
class UCameraShakeBase;

/**
 * 몹 공격 판정 시점. 몽타주 편집기에서 끌어 시각을 맞추고, 프리뷰에 판정 구가 보인다.
 * 팀 AM_MontageEvent와 같은 태그(Events.Montages.Attacks.Melee)를 보내므로 팀 GA와도 호환된다.
 * 판정 원점 = 발 + 위 GroundHeight + 앞 ForwardOffset + 옆 SideOffset (메시 앞 방향 = 메시 +Y)
 */
UCLASS(meta = (DisplayName = "Soul Mob Hit"))
class PROJECTSW_API USoulAnimNotify_MobHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	USoulAnimNotify_MobHit();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

	/** 메시 정보로 판정 원점을 구한다 (GA와 프리뷰가 같은 식을 쓴다) */
	FVector ComputeOrigin(const USkeletalMeshComponent* MeshComp) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (Units = "cm"))
	float ForwardOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (Units = "cm"))
	float SideOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (Units = "cm"))
	float GroundHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = "0", Units = "cm"))
	float Radius = 0.f;

	/** 이 타격의 피해 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = "0"))
	float DamageScale = 1.f;

	/** 비어 있으면 근접 판정. 있으면 판정 원점에서 대상 쪽으로 이 탄을 쏜다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	TSubclassOf<ASoulMobProjectile> ProjectileClass;

	/** 타격 순간 화면 흔들림 (보스 광역기 등). 비우면 없음. 플레이어가 맞지 않아도 판정 원점과의 거리로 세기를 줄여 흔든다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shake")
	TSubclassOf<UCameraShakeBase> CameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shake", meta = (ClampMin = "0"))
	float ShakeScale = 1.f;

	/** 판정 원점에서 이 거리 안이면 최대 세기 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shake", meta = (ClampMin = "0", Units = "cm"))
	float ShakeInnerRadius = 0.f;

	/** 이 거리에서 세기가 0이 된다. 안쪽과 바깥 반경이 둘 다 0이면 거리와 상관없이 최대 세기 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shake", meta = (ClampMin = "0", Units = "cm"))
	float ShakeOuterRadius = 0.f;

private:
	void PlayShake(UWorld* World, const FVector& Epicenter, const AActor* Owner, const UAnimSequenceBase* Animation) const;
};
