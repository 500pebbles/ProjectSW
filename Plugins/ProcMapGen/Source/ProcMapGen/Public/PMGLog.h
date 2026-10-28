#pragma once

#include "CoreMinimal.h"

PROCMAPGEN_API DECLARE_LOG_CATEGORY_EXTERN(LogProcMapGen, Log, All);

namespace PMG
{
	/** 경고 로그를 남기고 Warnings 목록에도 기록한다. */
	PROCMAPGEN_API void AddWarning(TArray<FString>& Warnings, const FString& Message);

	/** 에러 로그를 남긴다. */
	PROCMAPGEN_API void LogError(const FString& Message);
}
