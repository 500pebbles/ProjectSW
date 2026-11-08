#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoulRunGameComponent.generated.h"

class USoulRunConfig;

/** 런 GameMode의 컴포넌트 (BP_RunGameMode). DA_RunConfig를 하드 참조해 런 데이터가 쿡에 들어가게 하고, 다른 코드가 설정을 찾는 곳이다 */
UCLASS(ClassGroup = (Soul), meta = (BlueprintSpawnableComponent))
class PROJECTSW_API USoulRunGameComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USoulRunGameComponent();

	/** 지금 월드 GameMode의 런 설정. 런 GameMode가 아니면 null */
	UFUNCTION(BlueprintPure, Category = "Run", meta = (WorldContext = "WorldContextObject"))
	static USoulRunConfig* GetRunConfig(const UObject* WorldContextObject);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run")
	TObjectPtr<USoulRunConfig> RunConfig;
};
