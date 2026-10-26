// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "SoulAssetManager.generated.h"

UCLASS()
class PROJECTSW_API USoulAssetManager : public UAssetManager
{
	GENERATED_BODY()
	
public:		
	static USoulAssetManager& Get();
	
	virtual void StartInitialLoading() override;
};
