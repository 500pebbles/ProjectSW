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

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<USoulLootTable> LootTable;

	/** 드랍 흩뿌림 반경 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", Units = "cm"))
	float ScatterRadius = 0.f;

	/** 몹이면 사망 때 자동으로 굴린다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	bool bDropOnDeath = true;

private:
	void HandleDied(AActor* DeadActor);
};
