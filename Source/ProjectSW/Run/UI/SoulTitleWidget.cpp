#include "Run/UI/SoulTitleWidget.h"
#include "Run/Save/SoulHallOfFameSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void USoulTitleWidget::BuildDefaultTree()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;

	BackgroundImage = WidgetTree->ConstructWidget<UImage>();
	if (UCanvasPanelSlot* BoxSlot = Canvas->AddChildToCanvas(BackgroundImage))
	{
		BoxSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BoxSlot->SetOffsets(FMargin(0.f));
	}

	auto AddText = [this, Canvas](float AnchorY, int32 Size) -> UTextBlock*
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		Text->SetJustification(ETextJustify::Center);
		if (UCanvasPanelSlot* BoxSlot = Canvas->AddChildToCanvas(Text))
		{
			BoxSlot->SetAnchors(FAnchors(0.5f, AnchorY));
			BoxSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			BoxSlot->SetAutoSize(true);
		}
		return Text;
	};
	TitleLabel = AddText(0.35f, 72);
	PressKeyLabel = AddText(0.75f, 28);
	NoticeLabel = AddText(0.92f, 16);
}

void USoulTitleWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultTree();
	}
	if (BackgroundImage)
	{
		if (Background)
		{
			BackgroundImage->SetBrushFromTexture(Background);
			BackgroundImage->SetColorAndOpacity(FLinearColor::White);
		}
		else
		{
			BackgroundImage->SetColorAndOpacity(FLinearColor(0.02f, 0.02f, 0.03f, 1.f));
		}
	}
	if (TitleLabel)
	{
		TitleLabel->SetText(TitleText);
	}
	if (PressKeyLabel)
	{
		PressKeyLabel->SetText(PressKeyText);
	}
	if (NoticeLabel)
	{
		const USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this);
		NoticeLabel->SetText(FText::FromString(HoF ? HoF->GetLoadNotice() : FString()));
	}
}

void USoulTitleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Elapsed += InDeltaTime;
	if (OutDuration > 0.f)
	{
		OutTime += InDeltaTime;
		SetRenderOpacity(FMath::Clamp(1.f - OutTime / OutDuration, 0.f, 1.f));
		return;
	}
	if (PressKeyLabel && BlinkPeriod > 0.f)
	{
		const float Phase = FMath::Fmod(Elapsed, BlinkPeriod) / BlinkPeriod;
		PressKeyLabel->SetRenderOpacity(0.25f + 0.75f * (0.5f + 0.5f * FMath::Cos(Phase * 2.f * PI)));
	}
}

void USoulTitleWidget::PlayOut_Implementation(float Duration)
{
	OutDuration = FMath::Max(Duration, 0.01f);
	OutTime = 0.f;
	if (PressKeyLabel)
	{
		PressKeyLabel->SetRenderOpacity(1.f);
	}
}
