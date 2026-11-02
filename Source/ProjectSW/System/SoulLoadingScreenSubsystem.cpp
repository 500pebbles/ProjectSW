


#include "SoulLoadingScreenSubsystem.h"
#include "Utility/SoulLog.h"
#include "UI/Widget/SoulLoadingScreenWidget.h"
#include "Build/PMGMapBuilder.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"

USoulLoadingScreenSubsystem::USoulLoadingScreenSubsystem()
{
	SetTickableTickType(ETickableTickType::Never);
}

void USoulLoadingScreenSubsystem::ShowLoadingScreen()
{
	UGameViewportClient* ViewportClient = GetGameInstance()->GetGameViewportClient();

	if (LoadingScreenSlateWidget.IsValid() || !ViewportClient)
		return;

	if (!LoadingScreenWidget)
	{
		auto WidgetClass = LoadingScreenWidgetClass.LoadSynchronous();
		if (!WidgetClass)
		{
			SW_LOG_ERROR("USoulLoadingScreenSubsystem::ShowLoadingScreen : WidgetClass is null. (Check DefaultGame.ini)");
			return;
		}

		LoadingScreenWidget = CreateWidget<USoulLoadingScreenWidget>(GetGameInstance(), WidgetClass);
	}
	LoadingScreenWidget->UpdateLoadingStatus(EPMGBuildState::Idle, 0.f);

	LoadingScreenSlateWidget = LoadingScreenWidget->TakeWidget();
	ViewportClient->AddViewportWidgetContent(LoadingScreenSlateWidget.ToSharedRef(), LoadingScreenZOrder);
}

void USoulLoadingScreenSubsystem::HideLoadingScreen()
{
	if (!LoadingScreenSlateWidget.IsValid())
		return;

	UGameViewportClient* ViewportClient = GetGameInstance()->GetGameViewportClient();

	if (ViewportClient)
		ViewportClient->RemoveViewportWidgetContent(LoadingScreenSlateWidget.ToSharedRef());

	LoadingScreenSlateWidget.Reset();
}

void USoulLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	WorldInitializedActorsHandle = FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &ThisClass::HandleWorldInitializedActors);
	SetTickableTickType(ETickableTickType::Conditional);
}

void USoulLoadingScreenSubsystem::Deinitialize()
{
	SetTickableTickType(ETickableTickType::Never);
	FWorldDelegates::OnWorldInitializedActors.Remove(WorldInitializedActorsHandle);
	HideLoadingScreen();
	Super::Deinitialize();
}

void USoulLoadingScreenSubsystem::Tick(float DeltaTime)
{
	const bool bShouldShow = MapBuilder.IsValid() && MapBuilder->IsGenerating();

	if (!bShouldShow)
	{
		HideLoadingScreen();
		return;
	}

	ShowLoadingScreen();

	// 띄우기 실패
	if (!LoadingScreenSlateWidget.IsValid())
		return;

	int32 RequestedCount = 0;
	int32 LoadedCount = 0;
	MapBuilder->GetClassLoadProgress(LoadedCount, RequestedCount);

	int32 CompletedCount = 0;
	int32 TotalCount = 0;
	MapBuilder->GetBuildProgress(CompletedCount, TotalCount);

	const float Total = RequestedCount * ClassLoadFactor + TotalCount;
	const float Percent = Total > 0.f ? (LoadedCount * ClassLoadFactor + CompletedCount) / Total : 0.f;

	LoadingScreenWidget->UpdateLoadingStatus(MapBuilder->GetBuildState(), Percent);
}

TStatId USoulLoadingScreenSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USoulLoadingScreenSubsystem, STATGROUP_Tickables);
}

bool USoulLoadingScreenSubsystem::IsTickable() const
{
	// 뷰포트가 없으면 위젯을 붙일 수 없으니 Tick할 이유가 없다
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance && GameInstance->GetGameViewportClient();
}

bool USoulLoadingScreenSubsystem::IsTickableWhenPaused() const
{
	return true;
}

void USoulLoadingScreenSubsystem::HandleWorldInitializedActors(const FActorsInitializedParams& Params)
{
	if (!Params.World || Params.World->GetGameInstance() != GetGameInstance())
	{
		return;
	}
	
	TActorIterator<APMGMapBuilder> Iter(Params.World);

	MapBuilder = Iter ? *Iter : nullptr;
}
