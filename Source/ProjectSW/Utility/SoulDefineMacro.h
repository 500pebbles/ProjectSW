#pragma once

#define DEBUG_MSG(Format, ...) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, FString::Printf(TEXT(Format), ##__VA_ARGS__))