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
#include "Run/Presentation/SoulPresentationData.h"
#include "Run/UI/SoulPresentationWidgets.h"
#include "Engine/GameViewportClient.h"
#include "Containers/Ticker.h"

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

void USoulTransitionSubsystem::ShowLoading(FName EntryId)
{
	const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(GetGameInstance()->GetWorld());
	const USoulPresentationData* Data = Config ? Config->Presentation.Get() : nullptr;
	const FSoulLoadingEntry* Entry = Data ? Data->FindLoading(EntryId) : nullptr;
	UGameViewportClient* Viewport = GetGameInstance()->GetGameViewportClient();
	if (!Entry || !Data->LoadingWidgetClass || !Viewport)
		return;
	if (!LoadingWidget)
	{
		LoadingWidget = CreateWidget<USoulRunLoadingScreenWidget>(GetGameInstance(), Data->LoadingWidgetClass);
		if (!LoadingWidget)
			return;
		LoadingSlate = LoadingWidget->TakeWidget();
		Viewport->AddViewportWidgetContent(LoadingSlate.ToSharedRef(), 1000);
		LoadingWidget->SetRenderOpacity(Entry->FadeIn > 0.f ? 0.f : 1.f);
		LoadingShownAt = FPlatformTime::Seconds();
	}
	LoadingWidget->Setup(*Entry);
	LoadingId = EntryId;
	LoadingFadeIn = Entry->FadeIn;
	LoadingMinShow = Entry->MinShow;
	LoadingFadeOut = Entry->FadeOut;
	bLoadingHiding = false;
	if (!LoadingTicker.IsValid())
	{
		LoadingTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::TickLoading));
	}
	SW_LOG("[LOADING] show %s", *EntryId.ToString());
}

void USoulTransitionSubsystem::HideLoading()
{
	if (!LoadingWidget || bLoadingHiding)
		return;
	bLoadingHiding = true;
	// 최소 표시 시간을 채운 뒤 사라진다
	LoadingHideAt = FMath::Max(FPlatformTime::Seconds(), LoadingShownAt + LoadingMinShow);
}

bool USoulTransitionSubsystem::TickLoading(float DeltaTime)
{
	if (!LoadingWidget)
	{
		LoadingTicker.Reset();
		return false;
	}
	const double Now = FPlatformTime::Seconds();
	const float Shown = static_cast<float>(Now - LoadingShownAt);
	// 게이지는 최소 표시 시간으로 찬다 (실제 생성이 더 빨라서)
	LoadingWidget->SetProgress(LoadingMinShow > 0.f ? Shown / LoadingMinShow : 1.f);
	if (!bLoadingHiding)
	{
		LoadingWidget->SetRenderOpacity(LoadingFadeIn > 0.f ? FMath::Clamp(Shown / LoadingFadeIn, 0.f, 1.f) : 1.f);
		return true;
	}
	if (Now < LoadingHideAt)
		return true;
	const float Out = static_cast<float>(Now - LoadingHideAt);
	if (Out < LoadingFadeOut)
	{
		LoadingWidget->SetRenderOpacity(1.f - Out / LoadingFadeOut);
		return true;
	}
	if (UGameViewportClient* Viewport = GetGameInstance()->GetGameViewportClient(); Viewport && LoadingSlate.IsValid())
	{
		Viewport->RemoveViewportWidgetContent(LoadingSlate.ToSharedRef());
	}
	SW_LOG("[LOADING] hide %s shown=%.2fs", *LoadingId.ToString(), Shown);
	LoadingSlate.Reset();
	LoadingWidget = nullptr;
	bLoadingHiding = false;
	LoadingTicker.Reset();
	return false;
}

void USoulTransitionSubsystem::TravelTo(UObject* WorldContextObject, TSoftObjectPtr<UWorld> Map, float FadeSec, FName LoadingEntry)
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
	if (!LoadingEntry.IsNone())
	{
		ShowLoading(LoadingEntry);
	}
	SW_LOG("[TRAVEL] %s fade=%.2f", *Map.GetAssetName(), FadeSec);
	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(World, [World, Map]()
	{
		// 페이드 중에 연 UI의 입력 모드가 다음 맵으로 넘어가지 않게 한 번 더
		PrepareForTravel(World->GetFirstPlayerController());
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
	TravelTo(WorldContextObject, Config->TownMap, Config->TravelFadeSec, TEXT("Town"));
}

void USoulTransitionSubsystem::TravelToDungeon(UObject* WorldContextObject)
{
	const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(WorldContextObject);
	if (!Config)
	{
		SW_LOG_WARNING("[TRAVEL] DA_RunConfig 없음");
		return;
	}
	TravelTo(WorldContextObject, Config->DungeonMap, Config->TravelFadeSec, TEXT("F1"));
}
