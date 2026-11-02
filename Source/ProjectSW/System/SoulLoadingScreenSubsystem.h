

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "SoulLoadingScreenSubsystem.generated.h"

class USoulLoadingScreenWidget;
class SWidget;
class APMGMapBuilder;
struct FActorsInitializedParams;

/**
 * 
 */
UCLASS(Config = Game)
class PROJECTSW_API USoulLoadingScreenSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
	
public:
	USoulLoadingScreenSubsystem();

	void ShowLoadingScreen();
	void HideLoadingScreen();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;


	// FTickableGameObject을(를) 통해 상속됨
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override;

private:
	void HandleWorldInitializedActors(const FActorsInitializedParams& Params);

private:
	UPROPERTY(Config)
	TSoftClassPtr<USoulLoadingScreenWidget> LoadingScreenWidgetClass;

	UPROPERTY()
	TObjectPtr<USoulLoadingScreenWidget> LoadingScreenWidget;

	TSharedPtr<SWidget> LoadingScreenSlateWidget;

	FDelegateHandle WorldInitializedActorsHandle;

	static constexpr int32 LoadingScreenZOrder = 999;

	// 클래스 로드와 스폰 로드 비율 정하기 위한 팩터
	static constexpr float ClassLoadFactor = 10.f;

	UPROPERTY()
	TWeakObjectPtr<APMGMapBuilder> MapBuilder;
};
