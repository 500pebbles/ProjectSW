#pragma once

#include "CoreMinimal.h"
#include "Actor/SoulProjectileBase.h"
#include "SoulMobProjectile.generated.h"

/**
 * 원거리 몹 탄 (BP_MobProjectile_<이름>). 팀 탄과 달리 벽과 바닥에 막히고, 무적(대시)이나 죽은 대상은 통과한다.
 * 피해 Spec은 USoulMobAttackAbility가 근접과 같은 곱으로 만들어 넘긴다.
 */
UCLASS()
class PROJECTSW_API ASoulMobProjectile : public ASoulProjectileBase
{
	GENERATED_BODY()

public:
	ASoulMobProjectile();

	/** 비행 속도 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0", Units = "cm/s"))
	float Speed = 0.f;

	/** 이 거리를 날면 사라진다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0", Units = "cm"))
	float MaxDistance = 0.f;

	/** 판정 반지름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0", Units = "cm"))
	float Radius = 0.f;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleStop(const FHitResult& ImpactResult);

	void Explode();
};
