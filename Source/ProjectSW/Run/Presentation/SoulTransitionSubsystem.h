#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SoulTransitionSubsystem.generated.h"

class APlayerController;

/**
 * 맵 이동 창구 (타이틀 → 마을, 게이트 → 던전, 결과창 → 마을). 단계 F에서 로딩 화면과 일러스트가 여기에 붙는다.
 * 모든 OpenLevel 직전 공통 규칙(2.3): UI 닫기 → 게임 입력 모드 → 커서 끔 → 시간 배율 1. 입력 모드는 뷰포트에 남아 LoadMap을 넘는다.
 */
UCLASS()
class PROJECTSW_API USoulTransitionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static USoulTransitionSubsystem* Get(const UObject* WorldContextObject);

	/** 공통 규칙 후 FadeSec 동안 검게 덮고 맵을 연다. 이미 이동 중이면 무시 */
	UFUNCTION(BlueprintCallable, Category = "Transition", meta = (WorldContext = "WorldContextObject"))
	void TravelTo(UObject* WorldContextObject, TSoftObjectPtr<UWorld> Map, float FadeSec, FName LoadingEntry = NAME_None);

	/** DA_RunConfig의 TownMap으로 */
	UFUNCTION(BlueprintCallable, Category = "Transition", meta = (WorldContext = "WorldContextObject"))
	void TravelToTown(UObject* WorldContextObject);

	/** DA_RunConfig의 DungeonMap으로 */
	UFUNCTION(BlueprintCallable, Category = "Transition", meta = (WorldContext = "WorldContextObject"))
	void TravelToDungeon(UObject* WorldContextObject);

	/** 로딩 화면을 띄운다 (Town, F1~F3). 이미 떠 있으면 내용만 바꾼다 */
	void ShowLoading(FName EntryId);

	/** 최소 표시 시간을 채운 뒤 사라진다 */
	void HideLoading();

	bool IsLoadingVisible() const { return LoadingWidget != nullptr; }

	/** OpenLevel 직전 공통 규칙 (Director도 쓴다) */
	static void PrepareForTravel(APlayerController* PC);

	bool IsTraveling() const { return bTraveling; }

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	void HandlePostLoadMap(UWorld* World);
	bool TickLoading(float DeltaTime);
	bool bTraveling = false;

	UPROPERTY(Transient)
	TObjectPtr<class USoulRunLoadingScreenWidget> LoadingWidget;

	TSharedPtr<class SWidget> LoadingSlate;
	FTSTicker::FDelegateHandle LoadingTicker;
	FName LoadingId;
	double LoadingShownAt = 0.0;
	double LoadingHideAt = 0.0;
	float LoadingFadeIn = 0.f;
	float LoadingMinShow = 0.f;
	float LoadingFadeOut = 0.f;
	bool bLoadingHiding = false;
};
