#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SoulMobPatternInterface.generated.h"

class UAnimMontage;

UINTERFACE(BlueprintType)
class USoulMobPatternInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 보스 패턴을 BP에서 정할 때 몹 BP의 Class Settings > Interfaces에 추가한다.
 * 구현하면 USoulMobComponent가 AttackRules 추첨 대신 이 함수를 부른다. null을 돌려주면 이번에는 공격하지 않는다.
 */
class PROJECTSW_API ISoulMobPatternInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mob")
	UAnimMontage* ChooseAttackMontage(AActor* Target, float Distance);
};
