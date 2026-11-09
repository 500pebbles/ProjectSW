#pragma once

#include "CoreMinimal.h"
#include "Run/Event/SoulRoomEventObject.h"
#include "GameplayTagContainer.h"
#include "SoulRoomEvent_Effect.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;

/**
 * 회복과 함정 이벤트 (BP_RoomEvent_Heal, _Trap). 폰 ASC에 GE를 SetByCaller로 건다.
 * 회복: 최대 체력 x Magnitude. 함정: Min(최대 체력 x Magnitude, 체력 - 1) (bCanKill이면 체력 - 1 제한 없음).
 */
UCLASS()
class PROJECTSW_API ASoulRoomEvent_Effect : public ASoulRoomEventObject
{
	GENERATED_BODY()

public:
	/** 함정과 지뢰가 같이 쓰는 비율 피해 계산 */
	static float ComputeRatioDamage(const UAbilitySystemComponent* ASC, float Ratio, bool bCanKill);

	/** 폰 ASC에 GE를 SetByCaller 값으로 건다 */
	static void ApplyMagnitudeEffect(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayEffect> Effect, FGameplayTag MagnitudeTag, float Value);

protected:
	virtual void ApplyEvent_Implementation(APawn* User) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event")
	TSubclassOf<UGameplayEffect> Effect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event")
	FGameplayTag MagnitudeTag;

	/** 켜면 피해형(함정)으로 계산한다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event")
	bool bIsDamage = false;

	/** 피해형일 때 죽일 수 있는지 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event")
	bool bCanKill = false;
};
