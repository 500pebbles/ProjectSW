#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SoulPresentationWidgets.generated.h"

class UAbilitySystemComponent;
class UBorder;
class UImage;
class UProgressBar;
class UTextBlock;
struct FSoulLoadingEntry;
struct FOnAttributeChangeData;

/** 런 로딩 화면 (WBP_RunLoadingScreen 부모). 일러스트, 층 제목과 부제, 팁, 게이지. 트리가 비면 기본 배치 */
UCLASS()
class PROJECTSW_API USoulRunLoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FSoulLoadingEntry& Entry);

	UFUNCTION(BlueprintNativeEvent, Category = "Loading")
	void SetProgress(float Progress);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Loading")
	TObjectPtr<UImage> Background;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Loading")
	TObjectPtr<UTextBlock> Text_Title;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Loading")
	TObjectPtr<UTextBlock> Text_Subtitle;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Loading")
	TObjectPtr<UTextBlock> Text_Tip;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Loading")
	TObjectPtr<UProgressBar> Bar_Progress;

private:
	void BuildDefaultTree();
};

/** 배너 (WBP_FloorBanner 부모). 제목과 부제, 들어오고 머물고 나간다 */
UCLASS()
class PROJECTSW_API USoulBannerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 띄운다. Hold 뒤 사라지고 스스로 지운다 */
	static USoulBannerWidget* ShowBanner(APlayerController* PC, TSubclassOf<USoulBannerWidget> WidgetClass, const FText& Title, const FText& Subtitle, const FLinearColor& Color, float Hold);

	UFUNCTION(BlueprintNativeEvent, Category = "Banner")
	void Play(const FText& Title, const FText& Subtitle, const FLinearColor& Color, float Hold);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Banner")
	TObjectPtr<UTextBlock> Text_Title;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Banner")
	TObjectPtr<UTextBlock> Text_Subtitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Banner", meta = (ClampMin = "0", Units = "s"))
	float FadeInTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Banner", meta = (ClampMin = "0", Units = "s"))
	float FadeOutTime = 0.f;

private:
	float Elapsed = 0.f;
	float HoldTime = 0.f;
	bool bPlaying = false;
};

/** 보스 인트로와 상단 보스 체력 (WBP_BossIntro 부모). 레터박스, WARNING, 이름, 스킵 힌트와 게이지, 체력 바 */
UCLASS()
class PROJECTSW_API USoulBossIntroWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 인트로 화면 (레터박스, 경고, 스킵 힌트) */
	UFUNCTION(BlueprintNativeEvent, Category = "Boss")
	void ShowIntro(const FText& Warning, const FText& SkipHint);

	UFUNCTION(BlueprintNativeEvent, Category = "Boss")
	void ShowNameCard(const FText& Name, const FText& Title);

	/** 인트로 끝: 레터박스와 글을 걷고 상단 체력 바 */
	UFUNCTION(BlueprintNativeEvent, Category = "Boss")
	void ShowFight();

	UFUNCTION(BlueprintNativeEvent, Category = "Boss")
	void SetSkipProgress(float Progress);

	/** 보스 체력 구독 */
	void BindBoss(UAbilitySystemComponent* BossASC, const FText& Name);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Boss")
	TObjectPtr<UBorder> Letterbox_Top;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Boss")
	TObjectPtr<UBorder> Letterbox_Bottom;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Boss")
	TObjectPtr<UTextBlock> Text_Warning;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Boss")
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Boss")
	TObjectPtr<UTextBlock> Text_SkipHint;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Boss")
	TObjectPtr<UProgressBar> Bar_SkipHold;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Boss")
	TObjectPtr<UTextBlock> Text_BossName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Boss")
	TObjectPtr<UProgressBar> Bar_BossHP;

private:
	void BuildDefaultTree();
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void RefreshHealth();

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle HealthHandle;
	FDelegateHandle MaxHealthHandle;
};
