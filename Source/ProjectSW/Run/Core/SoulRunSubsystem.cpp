#include "Run/Core/SoulRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Utility/SoulLog.h"

USoulRunSubsystem* USoulRunSubsystem::Get(const UObject* WorldContextObject)
{
	const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	return GameInstance ? GameInstance->GetSubsystem<USoulRunSubsystem>() : nullptr;
}

void USoulRunSubsystem::BeginRun()
{
	State = FSoulRunState();
	State.Floor = 1;
	State.Level = 1;
	State.Exp = 0;
	// 층 시드에 층 번호와 재시도 횟수를 더하므로 int32 끝에서 넘치지 않게 범위를 둔다
	State.RunSeed = FMath::RandRange(1, 1000000000);
	State.StartRealTime = FPlatformTime::Seconds();
	State.bActive = true;
	State.bRunEnded = false;

	SW_LOG("[RUN] BeginRun seed=%d", State.RunSeed);
}

void USoulRunSubsystem::EndRun(bool bSuccess)
{
	if (!State.bActive || State.bRunEnded)
	{
		SW_LOG("[RUN] EndRun(%d) 무시: active=%d ended=%d", bSuccess, State.bActive, State.bRunEnded);
		return;
	}

	State.bRunEnded = true;
	const double Elapsed = GetElapsedRealSeconds();
	SW_LOG("[RUN] EndRun success=%d floor=%d level=%d sec=%.1f", bSuccess, State.Floor, State.Level, Elapsed);

	OnRunEnded.Broadcast(bSuccess);

	State.Level = 1;
	State.Exp = 0;
}

void USoulRunSubsystem::SetFloor(int32 NewFloor)
{
	State.Floor = NewFloor;
}

double USoulRunSubsystem::GetElapsedRealSeconds() const
{
	return State.bActive ? FPlatformTime::Seconds() - State.StartRealTime : 0.0;
}
