#include "Run/UI/SoulHallOfFameBoardWidget.h"
#include "Run/Equipment/SoulEquipmentCatalog.h"
#include "Run/Save/SoulHallOfFameSubsystem.h"
#include "Utility/SoulLog.h"
#include "Engine/Texture2D.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
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
		Box_Rows = WidgetTree->ConstructWidget<UVerticalBox>();
		Box->AddChildToVerticalBox(Box_Rows)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}
	if (TitleLabel)
	{
		TitleLabel->SetText(Title);
	}
	Refresh();
}

void USoulHallOfFameBoardWidget::Refresh()
{
	if (!Box_Rows || !WidgetTree)
		return;
	Box_Rows->ClearChildren();
	auto MakeText = [this](const FText& Text)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = RowFontSize;
		Label->SetFont(Font);
		Label->SetText(Text);
		return Label;
	};

	const USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this);
	const TArray<FSoulHallOfFameEntry> Entries = HoF ? HoF->GetTopEntries(HoF->GetBoardRows()) : TArray<FSoulHallOfFameEntry>();
	if (Entries.IsEmpty())
	{
		Box_Rows->AddChildToVerticalBox(MakeText(EmptyText));
		return;
	}
	const USoulEquipmentCatalog* Catalog = USoulEquipmentCatalog::GetRunCatalog(this);
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const FSoulHallOfFameEntry& Entry = Entries[i];
		const int32 Seconds = FMath::RoundToInt(Entry.ClearSeconds);
		FFormatNamedArguments Args;
		Args.Add(TEXT("Rank"), FText::AsNumber(i + 1));
		Args.Add(TEXT("Name"), FText::FromString(Entry.PlayerName));
		Args.Add(TEXT("Score"), FText::AsNumber(Entry.Score));
		Args.Add(TEXT("Grade"), Entry.RankId.IsNone() ? FailGradeText : FText::FromName(Entry.RankId));
		Args.Add(TEXT("Time"), FText::FromString(FString::Printf(TEXT("%02d:%02d"), Seconds / 60, Seconds % 60)));

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		UVerticalBoxSlot* RowSlot = Box_Rows->AddChildToVerticalBox(Row);
		RowSlot->SetPadding(FMargin(0.f, 4.f));
		// 사용한 의상과 무기 아이콘 (#8, 결과창과 같은 조회)
		for (UTexture2D* Icon : { Catalog ? Catalog->FindCostumeIcon(Entry.CostumeId) : nullptr, Catalog ? Catalog->FindWeaponIcon(Entry.WeaponId) : nullptr })
		{
			UImage* Image = WidgetTree->ConstructWidget<UImage>();
			Image->SetDesiredSizeOverride(FVector2D(IconSize, IconSize));
			USoulEquipmentCatalog::SetIcon(Image, Icon);
			UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(Image);
			IconSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}
		UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(MakeText(FText::Format(RowFormat, Args)));
		TextSlot->SetVerticalAlignment(VAlign_Center);
		const UTexture2D* CostumeIcon = Catalog ? Catalog->FindCostumeIcon(Entry.CostumeId) : nullptr;
		const UTexture2D* WeaponIcon = Catalog ? Catalog->FindWeaponIcon(Entry.WeaponId) : nullptr;
		SW_LOG("[HOF] board row %d %s grade=%s costumeIcon=%s weaponIcon=%s", i + 1, *Entry.PlayerName, *FText::Format(FText::FromString(TEXT("{Grade}")), Args).ToString(),
			CostumeIcon ? *CostumeIcon->GetName() : TEXT("none"), WeaponIcon ? *WeaponIcon->GetName() : TEXT("none"));
	}
}
