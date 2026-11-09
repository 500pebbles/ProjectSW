#include "Run/UI/SoulPresentationWidgets.h"
#include "Run/Presentation/SoulPresentationData.h"
#include "GAS/Core/SoulAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"

namespace SoulPresentationUI
{
	UTextBlock* AddText(UWidgetTree* Tree, UCanvasPanel* Canvas, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position, int32 Size, ETextJustify::Type Justify)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		Text->SetJustification(Justify);
		Text->SetShadowOffset(FVector2D(2.f, 2.f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
		if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Text))
		{
			CanvasSlot->SetAnchors(Anchors);
			CanvasSlot->SetAlignment(Alignment);
			CanvasSlot->SetPosition(Position);
			CanvasSlot->SetAutoSize(true);
		}
		return Text;
	}

	UProgressBar* AddBar(UWidgetTree* Tree, UCanvasPanel* Canvas, const FAnchors& Anchors, const FMargin& Offsets, const FLinearColor& Color)
	{
		UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>();
		Bar->SetFillColorAndOpacity(Color);
		if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Bar))
		{
			CanvasSlot->SetAnchors(Anchors);
			CanvasSlot->SetOffsets(Offsets);
		}
		return Bar;
	}
}

// ---------- 로딩 화면 ----------

void USoulRunLoadingScreenWidget::BuildDefaultTree()
{
	using namespace SoulPresentationUI;
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;
	Background = WidgetTree->ConstructWidget<UImage>();
	if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Background))
	{
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		CanvasSlot->SetOffsets(FMargin(0.f));
	}
	// 좌측 상단 제목, 하단 게이지와 팁
	Text_Title = AddText(WidgetTree, Canvas, FAnchors(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(80.f, 60.f), 64, ETextJustify::Left);
	Text_Subtitle = AddText(WidgetTree, Canvas, FAnchors(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(84.f, 150.f), 28, ETextJustify::Left);
	Text_Tip = AddText(WidgetTree, Canvas, FAnchors(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(0.f, -70.f), 20, ETextJustify::Center);
	Bar_Progress = AddBar(WidgetTree, Canvas, FAnchors(0.1f, 1.f, 0.9f, 1.f), FMargin(0.f, -50.f, 0.f, 12.f), FLinearColor::White);
}

void USoulRunLoadingScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultTree();
	}
}

void USoulRunLoadingScreenWidget::Setup(const FSoulLoadingEntry& Entry)
{
	if (Background)
	{
		if (Entry.Illustration)
		{
			Background->SetBrushFromTexture(Entry.Illustration);
			Background->SetColorAndOpacity(FLinearColor::White);
		}
		else
		{
			// 일러스트가 없으면 층 색을 아주 어둡게
			Background->SetColorAndOpacity(FLinearColor(Entry.Color.R * 0.08f, Entry.Color.G * 0.08f, Entry.Color.B * 0.08f, 1.f));
		}
	}
	if (Text_Title)
	{
		Text_Title->SetText(Entry.Title);
		Text_Title->SetColorAndOpacity(FSlateColor(Entry.Color));
	}
	if (Text_Subtitle)
	{
		Text_Subtitle->SetText(Entry.Subtitle);
	}
	if (Text_Tip)
	{
		Text_Tip->SetText(Entry.Tip);
		Text_Tip->SetVisibility(Entry.Tip.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (Bar_Progress)
	{
		Bar_Progress->SetFillColorAndOpacity(Entry.Color);
	}
	SetProgress(0.f);
}

void USoulRunLoadingScreenWidget::SetProgress_Implementation(float Progress)
{
	if (Bar_Progress)
	{
		Bar_Progress->SetPercent(FMath::Clamp(Progress, 0.f, 1.f));
	}
}

// ---------- 배너 ----------

USoulBannerWidget* USoulBannerWidget::ShowBanner(APlayerController* PC, TSubclassOf<USoulBannerWidget> WidgetClass, const FText& Title, const FText& Subtitle, const FLinearColor& Color, float Hold)
{
	if (!PC || !WidgetClass || Title.IsEmpty())
		return nullptr;
	USoulBannerWidget* Banner = CreateWidget<USoulBannerWidget>(PC, WidgetClass);
	if (Banner)
	{
		Banner->AddToViewport(50);
		Banner->Play(Title, Subtitle, Color, Hold);
	}
	return Banner;
}

void USoulBannerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		using namespace SoulPresentationUI;
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Canvas;
		Text_Title = AddText(WidgetTree, Canvas, FAnchors(0.5f, 0.3f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector, 64, ETextJustify::Center);
		Text_Subtitle = AddText(WidgetTree, Canvas, FAnchors(0.5f, 0.3f), FVector2D(0.5f, 0.f), FVector2D(0.f, 50.f), 26, ETextJustify::Center);
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void USoulBannerWidget::Play_Implementation(const FText& Title, const FText& Subtitle, const FLinearColor& Color, float Hold)
{
	if (Text_Title)
	{
		Text_Title->SetText(Title);
		Text_Title->SetColorAndOpacity(FSlateColor(Color));
	}
	if (Text_Subtitle)
	{
		Text_Subtitle->SetText(Subtitle);
		Text_Subtitle->SetVisibility(Subtitle.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	Elapsed = 0.f;
	HoldTime = Hold;
	bPlaying = true;
	SetRenderOpacity(FadeInTime > 0.f ? 0.f : 1.f);
}

void USoulBannerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bPlaying)
		return;
	Elapsed += InDeltaTime;
	if (Elapsed < FadeInTime)
	{
		SetRenderOpacity(Elapsed / FadeInTime);
	}
	else if (Elapsed < FadeInTime + HoldTime)
	{
		SetRenderOpacity(1.f);
	}
	else if (Elapsed < FadeInTime + HoldTime + FadeOutTime)
	{
		SetRenderOpacity(1.f - (Elapsed - FadeInTime - HoldTime) / FMath::Max(FadeOutTime, 0.01f));
	}
	else
	{
		bPlaying = false;
		RemoveFromParent();
	}
}

// ---------- 보스 인트로와 체력 ----------

void USoulBossIntroWidget::BuildDefaultTree()
{
	using namespace SoulPresentationUI;
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;
	auto AddBox = [this, Canvas](const FAnchors& Anchors, const FMargin& Offsets)
	{
		UBorder* Box = WidgetTree->ConstructWidget<UBorder>();
		Box->SetBrushColor(FLinearColor::Black);
		if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Box))
		{
			CanvasSlot->SetAnchors(Anchors);
			CanvasSlot->SetOffsets(Offsets);
		}
		return Box;
	};
	Letterbox_Top = AddBox(FAnchors(0.f, 0.f, 1.f, 0.f), FMargin(0.f, 0.f, 0.f, 110.f));
	Letterbox_Bottom = AddBox(FAnchors(0.f, 1.f, 1.f, 1.f), FMargin(0.f, -110.f, 0.f, 110.f));
	Text_Warning = AddText(WidgetTree, Canvas, FAnchors(0.5f, 0.22f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector, 56, ETextJustify::Center);
	Text_Warning->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.2f, 0.15f)));
	Text_Name = AddText(WidgetTree, Canvas, FAnchors(0.08f, 0.72f), FVector2D(0.f, 0.5f), FVector2D::ZeroVector, 44, ETextJustify::Left);
	Text_SkipHint = AddText(WidgetTree, Canvas, FAnchors(1.f, 1.f), FVector2D(1.f, 1.f), FVector2D(-40.f, -40.f), 18, ETextJustify::Right);
	Bar_SkipHold = AddBar(WidgetTree, Canvas, FAnchors(1.f, 1.f), FMargin(-260.f, -30.f, 220.f, 6.f), FLinearColor::White);
	Text_BossName = AddText(WidgetTree, Canvas, FAnchors(0.5f, 0.f), FVector2D(0.5f, 0.f), FVector2D(0.f, 24.f), 22, ETextJustify::Center);
	Bar_BossHP = AddBar(WidgetTree, Canvas, FAnchors(0.25f, 0.f, 0.75f, 0.f), FMargin(0.f, 58.f, 0.f, 16.f), FLinearColor(0.85f, 0.12f, 0.1f));
}

void USoulBossIntroWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultTree();
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	for (UWidget* Widget : TArray<UWidget*>{ Text_Warning, Text_Name, Text_SkipHint, Bar_SkipHold, Text_BossName, Bar_BossHP, Letterbox_Top, Letterbox_Bottom })
	{
		if (Widget)
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void USoulBossIntroWidget::ShowIntro_Implementation(const FText& Warning, const FText& SkipHint)
{
	for (UWidget* Widget : TArray<UWidget*>{ Letterbox_Top, Letterbox_Bottom, Text_Warning })
	{
		if (Widget)
		{
			Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	if (Text_Warning)
	{
		Text_Warning->SetText(Warning);
	}
	const bool bSkip = !SkipHint.IsEmpty();
	if (Text_SkipHint)
	{
		Text_SkipHint->SetText(SkipHint);
		Text_SkipHint->SetVisibility(bSkip ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (Bar_SkipHold)
	{
		Bar_SkipHold->SetPercent(0.f);
		Bar_SkipHold->SetVisibility(bSkip ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void USoulBossIntroWidget::ShowNameCard_Implementation(const FText& Name, const FText& Title)
{
	if (Text_Warning)
	{
		Text_Warning->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_Name)
	{
		Text_Name->SetText(Title.IsEmpty() ? Name : FText::Format(FText::FromString(TEXT("{0}\n{1}")), Title, Name));
		Text_Name->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void USoulBossIntroWidget::ShowFight_Implementation()
{
	for (UWidget* Widget : TArray<UWidget*>{ Letterbox_Top, Letterbox_Bottom, Text_Warning, Text_Name, Text_SkipHint, Bar_SkipHold })
	{
		if (Widget)
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	for (UWidget* Widget : TArray<UWidget*>{ Text_BossName, Bar_BossHP })
	{
		if (Widget)
		{
			Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	RefreshHealth();
}

void USoulBossIntroWidget::SetSkipProgress_Implementation(float Progress)
{
	if (Bar_SkipHold)
	{
		Bar_SkipHold->SetPercent(FMath::Clamp(Progress, 0.f, 1.f));
	}
}

void USoulBossIntroWidget::BindBoss(UAbilitySystemComponent* BossASC, const FText& Name)
{
	BoundASC = BossASC;
	if (Text_BossName)
	{
		Text_BossName->SetText(Name);
	}
	if (BossASC)
	{
		HealthHandle = BossASC->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetHealthAttribute()).AddUObject(this, &ThisClass::HandleHealthChanged);
		MaxHealthHandle = BossASC->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &ThisClass::HandleHealthChanged);
	}
	RefreshHealth();
}

void USoulBossIntroWidget::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	RefreshHealth();
}

void USoulBossIntroWidget::RefreshHealth()
{
	const UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC || !Bar_BossHP)
		return;
	const float Max = ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute());
	Bar_BossHP->SetPercent(Max > 0.f ? ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute()) / Max : 0.f);
}

void USoulBossIntroWidget::NativeDestruct()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetHealthAttribute()).Remove(HealthHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthHandle);
	}
	Super::NativeDestruct();
}
