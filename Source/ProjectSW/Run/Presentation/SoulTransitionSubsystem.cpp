#include "Run/Presentation/SoulTransitionSubsystem.h"
#include "Run/Core/SoulRunConfig.h"
#include "Run/Core/SoulRunGameComponent.h"
#include "Run/Interact/SoulInteractionComponent.h"
#include "Player/SoulPlayerController.h"
#include "UI/HUD/SoulHUD.h"
#include "Utility/SoulLog.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

USoulTransitionSubsystem* USoulTransitionSubsystem::Get(const UObject* WorldContextObject)
{
	const UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject);
	return GI ? GI->GetSubsystem<USoulTransitionSubsystem>() : nullptr;
}

void USoulTransitionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
}

void USoulTransitionSubsystem::HandlePostLoadMap(UWorld* World)
{
	bTraveling = false;
}

void USoulTransitionSubsystem::PrepareForTravel(APlayerController* PC)
{
	if (!PC)
		return;
	if (const ASoulHUD* HUD = Cast<ASoulHUD>(PC->GetHUD()); HUD && HUD->IsMapOpen())
	{
		if (ASoulPlayerController* SoulPC = Cast<ASoulPlayerController>(PC))
		{
			SoulPC->ToggleMap();
		}
	}
	if (USoulInteractionComponent* Interaction = USoulInteractionComponent::Find(PC->GetPawn()))
	{
		Interaction->CloseInventoryUI();
	}
	PC->FlushPressedKeys();
	PC->SetInputMode(FInputModeGameOnly());
	PC->SetShowMouseCursor(false);
	UGameplayStatics::SetGlobalTimeDilation(PC, 1.f);
}

void USoulTransitionSubsystem::TravelTo(UObject* WorldContextObject, TSoftObjectPtr<UWorld> Map, float FadeSec)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World || Map.IsNull() || bTraveling)
	{
		SW_LOG_WARNING("[TRAVEL] 무시: world=%d map=%s traveling=%d", World != nullptr, *Map.ToString(), bTraveling);
		return;
	}
	bTraveling = true;
	APlayerController* PC = World->GetFirstPlayerController();
	PrepareForTravel(PC);
	if (PC && PC->PlayerCameraManager && FadeSec > 0.f)
	{
		PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, FadeSec, FLinearColor::Black, false, true);
	}
	SW_LOG("[TRAVEL] %s fade=%.2f", *Map.GetAssetName(), FadeSec);
	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(World, [World, Map]()
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(World, Map);
	}), FMath::Max(FadeSec, 0.01f), false);
}

void USoulTransitionSubsystem::TravelToTown(UObject* WorldContextObject)
{
	const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(WorldContextObject);
	if (!Config)
	{
		SW_LOG_WARNING("[TRAVEL] DA_RunConfig 없음");
		return;
	}
	TravelTo(WorldContextObject, Config->TownMap, Config->TravelFadeSec);
}

void USoulTransitionSubsystem::TravelToDungeon(UObject* WorldContextObject)
{
	const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(WorldContextObject);
	if (!Config)
	{
		SW_LOG_WARNING("[TRAVEL] DA_RunConfig 없음");
		return;
	}
	TravelTo(WorldContextObject, Config->DungeonMap, Config->TravelFadeSec);
}
