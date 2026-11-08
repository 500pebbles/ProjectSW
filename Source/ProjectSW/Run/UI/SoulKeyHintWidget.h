#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SoulKeyHintWidget.generated.h"

class APlayerController;
class UInputAction;
class UTextBlock;
class UBorder;

/**
 * 작은 키 안내 (지도 키 안내, 포탈 표시). WBP 트리가 비면 코드 기본 트리(반투명 상자 + 글자)를 만든다.
 * 키 이름은 GetActionKeyText로 Enhanced Input 매핑에서 읽는다.
 */
UCLASS()
class PROJECTSW_API USoulKeyHintWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hint")
	void SetHint(const FText& InText, FLinearColor InColor);

	/** 글자 크기를 바꾼다 (생성 뒤에도 다시 적용) */
	UFUNCTION(BlueprintCallable, Category = "Hint")
	void SetFontSize(int32 InSize);

	/** 액션에 매핑된 첫 키 이름. 없으면 Fallback */
	static FText GetActionKeyText(const APlayerController* PC, const UInputAction* Action, const FText& Fallback);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hint", meta = (ClampMin = "8"))
	int32 FontSize = 18;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hint")
	FLinearColor BackgroundColor = FLinearColor(0.f, 0.f, 0.f, 0.55f);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Hint")
	TObjectPtr<UTextBlock> Text_Hint;
};
