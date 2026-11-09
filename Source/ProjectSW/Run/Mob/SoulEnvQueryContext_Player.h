#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "SoulEnvQueryContext_Player.generated.h"

/** EQS 컨텍스트: 플레이어 폰 (팀 EQS_PlayerContext는 BP_SoulPlayerCharacter만 찾아 치이에서 비어 있다) */
UCLASS(meta = (DisplayName = "Soul Player Pawn"))
class PROJECTSW_API USoulEnvQueryContext_Player : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
