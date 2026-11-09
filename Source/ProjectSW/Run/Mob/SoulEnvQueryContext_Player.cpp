#include "Run/Mob/SoulEnvQueryContext_Player.h"

#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "Kismet/GameplayStatics.h"

void USoulEnvQueryContext_Player::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(QueryInstance.Owner.Get(), 0))
	{
		UEnvQueryItemType_Actor::SetContextHelper(ContextData, Player);
	}
}
