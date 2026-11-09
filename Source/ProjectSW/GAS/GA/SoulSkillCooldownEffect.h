#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "SoulSkillCooldownEffect.generated.h"

/**
 * 스킬 쿨타임 GE. 지속시간은 SetByCaller(SoulTags.Cooldown.Duration), 쿨타임 태그는 스킬이 Spec에 동적으로 붙인다.
 * 숫자가 없는 배관이라 C++로 둔다 (BP로 만들면 SetByCaller 피커에 태그가 안 보일 수 있다).
 */
UCLASS()
class PROJECTSW_API USoulSkillCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USoulSkillCooldownEffect();
};
