#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSW, Log, All);

#define SW_LOG(Format, ...) UE_LOG(LogSW, Log, TEXT(Format), ##__VA_ARGS__)
#define SW_LOG_WARNING(Format, ...) UE_LOG(LogSW, Warning, TEXT(Format), ##__VA_ARGS__)
#define SW_LOG_ERROR(Format, ...) UE_LOG(LogSW, Error, TEXT(Format), ##__VA_ARGS__)