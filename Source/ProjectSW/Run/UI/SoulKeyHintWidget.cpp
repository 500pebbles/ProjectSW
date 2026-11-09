#include "Run/UI/SoulKeyHintWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"

void USoulKeyHintWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Root"));
		Root->SetBrushColor(BackgroundColor);
		Root->SetPadding(FMargin(12.f, 5.f));
		WidgetTree->RootWidget = Root;
		Text_Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Hint"));
		FSlateFontInfo Font = Text_Hint->GetFont();
		Font.Size = FontSize;
		Text_Hint->SetFont(Font);
		Text_Hint->SetShadowOffset(FVector2D(1.f, 1.f));
		Text_Hint->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
		Root->SetContent(Text_Hint);
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void USoulKeyHintWidget::SetHint(const FText& InText, FLinearColor InColor)
{
	if (Text_Hint)
	{
		Text_Hint->SetText(InText);
		Text_Hint->SetColorAndOpacity(FSlateColor(InColor));
	}
}

void USoulKeyHintWidget::SetFontSize(int32 InSize)
{
	FontSize = FMath::Max(InSize, 8);
	if (Text_Hint)
	{
		FSlateFontInfo Font = Text_Hint->GetFont();
		Font.Size = FontSize;
		Text_Hint->SetFont(Font);
	}
}

FText USoulKeyHintWidget::GetActionKeyText(const APlayerController* PC, const UInputAction* Action, const FText& Fallback)
{
	const UEnhancedInputLocalPlayerSubsystem* Input = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Input || !Action)
		return Fallback;
	const TArray<FKey> Keys = Input->QueryKeysMappedToAction(Action);
	return Keys.IsEmpty() ? Fallback : Keys[0].GetDisplayName(false);
}
