#pragma once

#include "CoreMinimal.h"
#include "Item/Effect/ItemEffect.h"
#include "GameplayTagContainer.h"
#include "SoulItemEffect_ApplyEffect.generated.h"

class UGameplayEffect;

/**
 * 아이템 사용 효과: 사용자 ASC에 GE를 건다 (포션). SetByCaller 값 = 최대 체력 x MaxHealthRatio + FlatMagnitude.
 * DA_Item_*의 Effects에 넣는다. 효과 객체는 DA가 공유하므로 상태를 갖지 않는다.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class PROJECTSW_API USoulItemEffect_ApplyEffect : public UItemEffect
{
	GENERATED_BODY()

public:
	virtual void ActivateItemEffect_Implementation(AActor* User, UItemData* Item) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> Effect;

	/** SetByCaller 태그 (GE_Item_*의 Modifier와 같은 태그) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	FGameplayTag MagnitudeTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect", meta = (ClampMin = "0"))
	float MaxHealthRatio = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	float FlatMagnitude = 0.f;
};
