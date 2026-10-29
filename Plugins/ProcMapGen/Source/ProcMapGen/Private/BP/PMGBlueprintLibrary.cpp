#include "BP/PMGBlueprintLibrary.h"
#include "PMGGenerator.h"
#include "PMGLog.h"

FPMGResult UPMGBlueprintLibrary::GenerateMap(const UPMGSettings* Settings, int32 Seed)
{
	if (!Settings)
	{
		FPMGResult Result;
		Result.Seed = Seed;
		Result.FailReason = TEXT("Settings가 None");
		PMG::LogError(TEXT("GenerateMap: ") + Result.FailReason);
		return Result;
	}
	return PMG::GenerateMap(*Settings, Seed);
}
