#include "Run/UI/SoulHallOfFameBoardWidget.h"
#include "Run/Save/SoulHallOfFameSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void USoulHallOfFameBoardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Root"));
		Root->SetBrushColor(FLinearColor(0.05f, 0.04f, 0.03f, 0.85f));
		Root->SetPadding(FMargin(24.f));
		WidgetTree->RootWidget = Root;
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
		Root->SetContent(Box);
		TitleLabel = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo TitleFont = TitleLabel->GetFont();
		TitleFont.Size = 40;
		TitleLabel->SetFont(TitleFont);
		TitleLabel->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.85f, 0.4f)));
		Box->AddChildToVerticalBox(TitleLabel)->SetHorizontalAlignment(HAlign_Center);
		RowsLabel = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo RowFont = RowsLabel->GetFont();
		RowFont.Size = 26;
		RowsLabel->SetFont(RowFont);
		Box->AddChildToVerticalBox(RowsLabel)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}
	if (TitleLabel)
	{
		TitleLabel->SetText(Title);
	}
	Refresh();
}

void USoulHallOfFameBoardWidget::Refresh()
{
	if (!RowsLabel)
		return;
	const USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this);
	const TArray<FSoulHallOfFameEntry> Entries = HoF ? HoF->GetTopEntries(HoF->GetBoardRows()) : TArray<FSoulHallOfFameEntry>();
	if (Entries.IsEmpty())
	{
		RowsLabel->SetText(EmptyText);
		return;
	}
	FString Lines;
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const FSoulHallOfFameEntry& Entry = Entries[i];
		const int32 Seconds = FMath::RoundToInt(Entry.ClearSeconds);
		FFormatNamedArguments Args;
		Args.Add(TEXT("Rank"), FText::AsNumber(i + 1));
		Args.Add(TEXT("Name"), FText::FromString(Entry.PlayerName));
		Args.Add(TEXT("Score"), FText::AsNumber(Entry.Score));
		Args.Add(TEXT("Grade"), FText::FromName(Entry.RankId));
		Args.Add(TEXT("Time"), FText::FromString(FString::Printf(TEXT("%02d:%02d"), Seconds / 60, Seconds % 60)));
		Lines += FText::Format(RowFormat, Args).ToString() + TEXT("\n");
	}
	RowsLabel->SetText(FText::FromString(Lines.TrimEnd()));
}
