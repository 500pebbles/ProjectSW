#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SoulHallOfFameBoardWidget.generated.h"

class UTextBlock;
class UVerticalBox;

/** 마을 보드 (WBP_HallOfFameBoard 부모). 제목 + 상위 행. 트리가 비면 기본 배치 */
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

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HallOfFame")
	TObjectPtr<UTextBlock> TitleLabel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HallOfFame")
	TObjectPtr<UTextBlock> RowsLabel;
};
