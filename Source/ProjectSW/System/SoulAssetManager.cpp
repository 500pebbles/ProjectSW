// 


#include "SoulAssetManager.h"

USoulAssetManager& USoulAssetManager::Get()
{
	check(GEngine);	
	
	USoulAssetManager* SoulAssetManager =  Cast<USoulAssetManager>(GEngine->AssetManager);
	return *SoulAssetManager;
}

void USoulAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
}
