#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SoulLevelBarWidget.generated.h"

class UProgressBar;
class UTextBlock;
class USoulProgressionComponent;

/** 런 경험치 바 (WBP_LevelBar의 부모). 위젯 이름만 맞으면 디자이너에서 모양을 바꿔도 연결된다 */
UCLASS()
class PROJECTSW_API USoulLevelBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Bind(USoulProgressionComponent* InProgression);

	/** 값이 바뀔 때. BP에서 덮어써 연출을 붙인다 */
	UFUNCTION(BlueprintNativeEvent, Category = "Level")
	void Refresh(int32 Level, int32 Exp, int32 ExpToNext);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> Bar_Exp;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Level;

private:
	UFUNCTION()
	void HandleChanged(int32 Level, int32 Exp, int32 ExpToNext);
};
