#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SoulTitleWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

/** 타이틀 (WBP_TitleScreen 부모). 배경, 제목, PRESS ANY KEY 깜빡임, 세이브 안내 한 줄. 트리가 비면 기본 배치 */
UCLASS()
class PROJECTSW_API USoulTitleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 키를 받았을 때 (깜빡임 멈춤, 서서히 사라짐) */
	UFUNCTION(BlueprintNativeEvent, Category = "Title")
	void PlayOut(float Duration);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title")
	FText TitleText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title")
	FText PressKeyText;

	/** 배경 키아트 (T_Title_KeyArt, 단계 F). 비면 검은 배경 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title")
	TObjectPtr<UTexture2D> Background;

	/** 깜빡임 주기 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title", meta = (ClampMin = "0", Units = "s"))
	float BlinkPeriod = 0.f;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Title")
	TObjectPtr<UImage> BackgroundImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Title")
	TObjectPtr<UTextBlock> TitleLabel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Title")
	TObjectPtr<UTextBlock> PressKeyLabel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Title")
	TObjectPtr<UTextBlock> NoticeLabel;

private:
	void BuildDefaultTree();
	float Elapsed = 0.f;
	float OutTime = 0.f;
	float OutDuration = 0.f;
};
