#include "Run/Core/SoulShots.h"
#include "Utility/SoulLog.h"

#include "Containers/Ticker.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace SoulShots
{
	static FString GetDir()
	{
		FString Dir;
#if !UE_BUILD_SHIPPING
		FParse::Value(FCommandLine::Get(), TEXT("SoulShotDir="), Dir);
#endif
		return Dir;
	}

	bool IsEnabled()
	{
		return !GetDir().IsEmpty();
	}

	static void TakeNow(const FString& Name)
	{
		const FString Path = FPaths::Combine(GetDir(), Name + TEXT(".png"));
		FScreenshotRequest::RequestScreenshot(Path, true, false);
		SW_LOG("[SHOT] %s", *Path);
	}

	void Take(const FString& Name, float DelaySec)
	{
		if (!IsEnabled())
			return;
		if (DelaySec <= 0.f)
		{
			TakeNow(Name);
			return;
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Name](float)
		{
			TakeNow(Name);
			return false;
		}), DelaySec);
	}
}
