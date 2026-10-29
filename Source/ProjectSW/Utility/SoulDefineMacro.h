#pragma once

/* Debug Section */
#define DEBUG_MSG(Format, ...) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, FString::Printf(TEXT(Format), ##__VA_ARGS__))

/* Collision Section */
#define ECC_Projectile ECollisionChannel::ECC_GameTraceChannel1