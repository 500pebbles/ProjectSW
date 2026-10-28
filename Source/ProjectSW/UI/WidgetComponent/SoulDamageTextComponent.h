// 

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "SoulDamageTextComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSW_API USoulDamageTextComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetDamageText(float Damage);
};