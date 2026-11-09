#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Run/Save/SoulHallOfFameTypes.h"
#include "SoulTownWidgets.generated.h"

class UProgressBar;
class UScrollBox;
class UTextBlock;
class USoulCostumeDefinition;
class USoulHallOfFamePanelWidget;

/** 의상 카드 (WBP_CostumeCard 부모). 이름, 수치 요약, 그 의상 최고 기록. 석상 위 월드 위젯, 포커스 때 표시 */
UCLASS()
class PROJECTSW_API USoulCostumeCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Card")
	void SetCostume(USoulCostumeDefinition* Costume);

	/** {Score} {Grade} {Name}. 기록이 없으면 NoRecordText */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FText BestFormat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FText NoRecordText;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Card")
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Card")
	TObjectPtr<UTextBlock> Text_Summary;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Card")
	TObjectPtr<UTextBlock> Text_Best;
};

/** 1위 석상 이름표 (WBP_ChampionPlate 부모). 글 한 줄 */
UCLASS()
class PROJECTSW_API USoulNamePlateWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPlateText(const FText& InText);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Plate")
	TObjectPtr<UTextBlock> Text_Plate;
};

/** 처음 실행 안내 (WBP_TownHint 부모). DisplaySec 뒤 사라진다 */
UCLASS()
class PROJECTSW_API USoulTownHintWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hint", meta = (MultiLine = true))
	FText HintText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hint", meta = (ClampMin = "0", Units = "s"))
	float DisplaySec = 8.f;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Hint")
	TObjectPtr<UTextBlock> Text_Hint;
};

/** 명예의 전당 패널의 행 버튼 (눌린 행 번호를 패널에 돌려준다) */
UCLASS()
class PROJECTSW_API USoulHoFRowButton : public UButton
{
	GENERATED_BODY()

public:
	void Bind(USoulHallOfFamePanelWidget* InPanel, int32 InIndex);

private:
	UFUNCTION()
	void HandleClicked();

	TWeakObjectPtr<USoulHallOfFamePanelWidget> Panel;
	int32 Index = INDEX_NONE;
};

/** 명예의 전당 패널 (WBP_HallOfFame 부모). 보드에서 F. 전체 목록, 점수와 시간 탭, 행 세부, 길게 눌러 초기화, Esc로 닫기 */
UCLASS()
class PROJECTSW_API USoulHallOfFamePanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	static USoulHallOfFamePanelWidget* Open(APlayerController* PC, TSubclassOf<USoulHallOfFamePanelWidget> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "HallOfFame")
	void Close();

	void SelectEntry(int32 Index);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame")
	FText TitleText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame")
	FText TabScoreText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame")
	FText TabTimeText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame")
	FText ResetText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame")
	FText CloseText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame")
	FText EmptyText;

	/** {Rank} {Name} {Score} {Grade} {Time} */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame")
	FText RowFormat;

	/** {Name} {Score} {Grade} {Time} {Level} {Kills} {Events} {Damage} {Costume} {Weapon} {Date} */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame")
	FText DetailFormat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame")
	FText FailGradeText;

	/** 초기화 버튼을 이만큼 누르고 있어야 지운다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame", meta = (ClampMin = "0", Units = "s"))
	float ResetHoldSec = 0.f;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HallOfFame")
	TObjectPtr<UScrollBox> Scroll_Rows;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HallOfFame")
	TObjectPtr<UTextBlock> Text_Detail;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HallOfFame")
	TObjectPtr<UButton> Button_TabScore;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HallOfFame")
	TObjectPtr<UButton> Button_TabTime;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HallOfFame")
	TObjectPtr<UButton> Button_Reset;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HallOfFame")
	TObjectPtr<UProgressBar> Bar_ResetHold;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HallOfFame")
	TObjectPtr<UButton> Button_Close;

private:
	void BuildDefaultTree();
	void Rebuild();
	FText FormatGrade(const FSoulHallOfFameEntry& Entry) const;

	UFUNCTION()
	void HandleTabScore();

	UFUNCTION()
	void HandleTabTime();

	TArray<FSoulHallOfFameEntry> Shown;
	bool bSortByTime = false;
	float ResetHeld = 0.f;
};
