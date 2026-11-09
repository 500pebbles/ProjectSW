#include "Run/Core/SoulRunGameComponent.h"
#include "Run/Core/SoulRunConfig.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"

USoulRunGameComponent::USoulRunGameComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

USoulRunConfig* USoulRunGameComponent::GetRunConfig(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	const USoulRunGameComponent* Component = GameMode ? GameMode->FindComponentByClass<USoulRunGameComponent>() : nullptr;
	return Component ? Component->RunConfig.Get() : nullptr;
}

bool USoulRunGameComponent::IsRunWorld(const UObject* WorldContextObject)
{
	return GetRunConfig(WorldContextObject) != nullptr;
}
