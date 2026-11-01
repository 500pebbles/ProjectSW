

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Build/PMGMapBuilder.h"
#include "SoulLoadingScreenWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTSW_API USoulLoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()
	

public:
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateLoadingStatus(EPMGBuildState BuildState, float Progress);
};
