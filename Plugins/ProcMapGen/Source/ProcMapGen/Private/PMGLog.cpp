#include "PMGLog.h"

DEFINE_LOG_CATEGORY(LogProcMapGen);

namespace PMG
{
	void AddWarning(TArray<FString>& Warnings, const FString& Message)
	{
		UE_LOG(LogProcMapGen, Warning, TEXT("%s"), *Message);
		Warnings.Add(Message);
	}

	void LogError(const FString& Message)
	{
		UE_LOG(LogProcMapGen, Error, TEXT("%s"), *Message);
	}
}
