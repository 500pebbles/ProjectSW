#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Run/Save/SoulHallOfFameTypes.h"
#include "SoulResultWidget.generated.h"

class UButton;
class UEditableText;
class UTextBlock;
class UVerticalBox;
class UWidget;

/**
 * 결과창 (WBP_Result 부모). 배너, 랭크 도장, 점수 카운트업, 내역, 시간, 레벨, 의상과 무기, 이름 입력(클리어만), 귀환 버튼.
 * WBP 트리가 비어 있으면 C++이 기본 배치를 만든다. 사람이 WBP에서 같은 이름의 위젯을 두면 그것을 쓴다(BindWidgetOptional).
 */
UCLASS()
class PROJECTSW_API USoulResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 결과를 채운다. EntryId가 유효하면 이름 입력을 보인다 */
	UFUNCTION(BlueprintCallable, Category = "Result")
	void Setup(const FSoulRunSummary& Summary, FGuid EntryId);

	/** 결과창을 띄우고 UI 입력으로 바꾼다 (Director가 부른다) */
	static USoulResultWidget* ShowResult(APlayerController* PC, TSubclassOf<USoulResultWidget> WidgetClass, const FSoulRunSummary& Summary, FGuid EntryId);

	/** 디버그: 이름 칸에 넣기 */
	void SetNameForTest(const FString& Name);

	/** 이름을 저장하고 마을로 */
	UFUNCTION(BlueprintCallable, Category = "Result")
	void ReturnToTown();

	/** 점수 카운트업 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result", meta = (ClampMin = "0", Units = "s"))
	float CountUpTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result")
	FText ClearBanner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result")
	FText FailBanner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result")
	FText ReturnLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result")
	FText NameLabel;

	/** 내역 항목 이름 (원장 키 → 표시 이름). 없으면 키 그대로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result")
	TMap<FName, FText> CategoryLabels;

	/** {Time} {Level} {Costume} {Weapon} {Kills} {Events} */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result")
	FText DetailFormat;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Result")
	TObjectPtr<UTextBlock> BannerText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Result")
	TObjectPtr<UTextBlock> RankText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Result")
	TObjectPtr<UTextBlock> ScoreText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Result")
	TObjectPtr<UTextBlock> BreakdownText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Result")
	TObjectPtr<UTextBlock> DetailText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Result")
	TObjectPtr<UWidget> NameRow;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Result")
	TObjectPtr<UEditableText> NameInput;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Result")
	TObjectPtr<UTextBlock> NameCountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Result")
	TObjectPtr<UButton> ReturnButton;

private:
	void BuildDefaultTree();
	UTextBlock* MakeText(UVerticalBox* Box, int32 Size, const FLinearColor& Color);

	UFUNCTION()
	void HandleReturnClicked();

	UFUNCTION()
	void HandleNameChanged(const FText& Text);

	UFUNCTION()
	void HandleNameCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	FSoulRunSummary Summary;
	FGuid EntryId;
	float CountUpElapsed = 0.f;
	bool bReturning = false;
};
