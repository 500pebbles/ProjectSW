#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SoulHallOfFameBoardWidget.generated.h"

class UTextBlock;
class UVerticalBox;

/** 마을 보드 (WBP_HallOfFameBoard 부모). 제목 + 상위 행(글과 의상, 무기 아이콘). 트리가 비면 기본 배치 */
UCLASS()
class PROJECTSW_API USoulHallOfFameBoardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HallOfFame")
	void Refresh();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame")
	FText Title;

	/** 기록이 없을 때 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame")
	FText EmptyText;

	/** {Rank} {Name} {Score} {Grade} {Time} */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame")
	FText RowFormat;

	/** 실패 기록의 등급 칸 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame")
	FText FailGradeText;

	/** 행 글자 크기와 아이콘 크기 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame", meta = (ClampMin = "1"))
	int32 RowFontSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame", meta = (ClampMin = "1"))
	float IconSize = 1.f;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HallOfFame")
	TObjectPtr<UTextBlock> TitleLabel;

	/** 행이 들어갈 상자 (Refresh가 비우고 다시 채운다) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HallOfFame")
	TObjectPtr<UVerticalBox> Box_Rows;
};
