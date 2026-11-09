#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoulLootDropComponent.generated.h"

class USoulLootTable;

/** 몹이 죽을 때 드랍 표를 굴린다 (BP_MobBase, 보스는 자식에서 표를 바꾼다). 표가 비어 있으면 아무것도 안 한다 */
UCLASS(ClassGroup = (Soul), meta = (BlueprintSpawnableComponent))
class PROJECTSW_API USoulLootDropComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USoulLootDropComponent();

	/** 지금 위치에서 표를 굴린다 (몹 사망, 상자 개봉) */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void DropLoot();

	/** 주어진 자리에서 표를 굴린다 (상자 DropOffset) */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void DropLootAt(const FVector& Location);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<USoulLootTable> LootTable;

	/** 드랍 흩뿌림 반경 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", Units = "cm"))
	float ScatterRadius = 0.f;

	/** 여러 개가 나올 때 서로 띄우는 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", Units = "cm"))
	float DropSpacing = 0.f;

	/** 바닥(NavMesh) 투영 범위 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FVector DropNavExtent = FVector::ZeroVector;

	/** 투영이 실패하면 플레이어 앞 이 거리에 놓는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", Units = "cm"))
	float DropFallbackForward = 0.f;

	/** 몹이면 사망 때 자동으로 굴린다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	bool bDropOnDeath = true;

private:
	void HandleDied(AActor* DeadActor);
};
