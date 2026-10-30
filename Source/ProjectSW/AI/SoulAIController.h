// 

#pragma once

#include "CoreMinimal.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "SoulAIController.generated.h"

class UBehaviorTreeComponent;

UCLASS()
class PROJECTSW_API ASoulAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	ASoulAIController();
	
protected:
	UPROPERTY()
	TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent;
};
