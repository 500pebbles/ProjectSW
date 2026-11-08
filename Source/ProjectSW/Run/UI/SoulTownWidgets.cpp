#include "Run/UI/SoulTownWidgets.h"
#include "Run/Equipment/SoulCostumeDefinition.h"
#include "Run/Equipment/SoulEquipmentCatalog.h"
#include "Run/Equipment/SoulEquipmentDefinition.h"
#include "Run/Save/SoulHallOfFameSubsystem.h"
#include "Utility/SoulLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace SoulTownUI
{
	UTextBlock* Text(UWidgetTree* Tree, int32 Size, const FLinearColor& Color = FLinearColor::White)
	{
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = Size;
		Label->SetFont(Font);
		Label->SetColorAndOpacity(FSlateColor(Color));
		return Label;
	}

	UButton* Button(UWidgetTree* Tree, const FText& Label, TSubclassOf<UButton> Class = UButton::StaticClass())
	{
		UButton* Btn = Tree->ConstructWidget<UButton>(Class);
		UTextBlock* Caption = Text(Tree, 18, FLinearColor::Black);
		Caption->SetText(Label);
		Btn->SetContent(Caption);
		return Btn;
	}

	FText FormatTime(float Seconds)
	{
		const int32 Total = FMath::Max(0, FMath::RoundToInt(Seconds));
		return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Total / 60, Total % 60));
	}
}

// ---------- 의상 카드 ----------

void USoulCostumeCardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Root"));
		Root->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.7f));
		Root->SetPadding(FMargin(14.f, 10.f));
		WidgetTree->RootWidget = Root;
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
		Root->SetContent(Box);
		Text_Name = SoulTownUI::Text(WidgetTree, 22, FLinearColor(1.f, 0.85f, 0.4f));
		Text_Summary = SoulTownUI::Text(WidgetTree, 15);
		Text_Best = SoulTownUI::Text(WidgetTree, 14, FLinearColor(0.8f, 0.8f, 0.8f));
		Box->AddChildToVerticalBox(Text_Name);
		Box->AddChildToVerticalBox(Text_Summary);
		Box->AddChildToVerticalBox(Text_Best);
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void USoulCostumeCardWidget::SetCostume_Implementation(USoulCostumeDefinition* Costume)
{
	if (!Costume)
		return;
	if (Text_Name)
	{
		Text_Name->SetText(Costume->DisplayName);
	}
	if (Text_Summary)
	{
		Text_Summary->SetText(Costume->StatSummary);
	}
	if (Text_Best)
	{
		// 이 의상으로 세운 최고 기록 (기록은 점수 순이라 처음 만나는 행)
		const USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this);
		const TArray<FSoulHallOfFameEntry> All = HoF ? HoF->GetTopEntries(HoF->GetEntryCount()) : TArray<FSoulHallOfFameEntry>();
		const FSoulHallOfFameEntry* Best = All.FindByPredicate([Costume](const FSoulHallOfFameEntry& E) { return E.CostumeId == Costume->GetFName() && E.bSuccess; });
		if (Best)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Score"), FText::AsNumber(Best->Score));
			Args.Add(TEXT("Grade"), FText::FromName(Best->RankId));
			Args.Add(TEXT("Name"), FText::FromString(Best->PlayerName));
			Text_Best->SetText(FText::Format(BestFormat, Args));
		}
		else
		{
			Text_Best->SetText(NoRecordText);
		}
	}
	SW_LOG("[TOWN] card %s best=%s", *Costume->GetName(), Text_Best ? *Text_Best->GetText().ToString() : TEXT("-"));
}

// ---------- 1위 이름표 ----------

void USoulNamePlateWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Root"));
		Root->SetBrushColor(FLinearColor(0.1f, 0.07f, 0.02f, 0.8f));
		Root->SetPadding(FMargin(16.f, 6.f));
		WidgetTree->RootWidget = Root;
		Text_Plate = SoulTownUI::Text(WidgetTree, 24, FLinearColor(1.f, 0.85f, 0.4f));
		Root->SetContent(Text_Plate);
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void USoulNamePlateWidget::SetPlateText(const FText& InText)
{
	if (Text_Plate)
	{
		Text_Plate->SetText(InText);
	}
}

// ---------- 처음 실행 안내 ----------

void USoulTownHintWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Root"));
		Root->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
		Root->SetPadding(FMargin(28.f, 16.f));
		Root->SetHorizontalAlignment(HAlign_Center);
		WidgetTree->RootWidget = Root;
		Text_Hint = SoulTownUI::Text(WidgetTree, 20);
		Text_Hint->SetJustification(ETextJustify::Center);
		Root->SetContent(Text_Hint);
	}
	if (Text_Hint)
	{
		Text_Hint->SetText(HintText);
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void USoulTownHintWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SW_LOG("[TOWN] hint shown sec=%.1f", DisplaySec);
	if (DisplaySec > 0.f && GetWorld())
	{
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() { RemoveFromParent(); }), DisplaySec, false);
	}
}

// ---------- 패널 행 버튼 ----------

void USoulHoFRowButton::Bind(USoulHallOfFamePanelWidget* InPanel, int32 InIndex)
{
	Panel = InPanel;
	Index = InIndex;
	OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
}

void USoulHoFRowButton::HandleClicked()
{
	if (Panel.IsValid())
	{
		Panel->SelectEntry(Index);
	}
}

// ---------- 명예의 전당 패널 ----------

USoulHallOfFamePanelWidget* USoulHallOfFamePanelWidget::Open(APlayerController* PC, TSubclassOf<USoulHallOfFamePanelWidget> WidgetClass)
{
	if (!PC || !WidgetClass)
		return nullptr;
	USoulHallOfFamePanelWidget* Panel = CreateWidget<USoulHallOfFamePanelWidget>(PC, WidgetClass);
	if (!Panel)
		return nullptr;
	Panel->AddToViewport(60);
	Panel->SetIsFocusable(true);
	PC->FlushPressedKeys();
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Panel->TakeWidget());
	PC->SetInputMode(Mode);
	PC->SetShowMouseCursor(true);
	SW_LOG("[HOF] panel open entries=%d", Panel->Shown.Num());
	return Panel;
}

void USoulHallOfFamePanelWidget::Close()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
	RemoveFromParent();
}

void USoulHallOfFamePanelWidget::BuildDefaultTree()
{
	using namespace SoulTownUI;
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Root"));
	Root->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	Root->SetPadding(FMargin(60.f, 40.f));
	WidgetTree->RootWidget = Root;
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	Root->SetContent(Box);
	UTextBlock* Title = Text(WidgetTree, 36, FLinearColor(1.f, 0.85f, 0.4f));
	Title->SetText(TitleText);
	Box->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);

	UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>();
	Box->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0.f, 12.f));
	Button_TabScore = Button(WidgetTree, TabScoreText);
	Button_TabTime = Button(WidgetTree, TabTimeText);
	Tabs->AddChildToHorizontalBox(Button_TabScore)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	Tabs->AddChildToHorizontalBox(Button_TabTime);

	UHorizontalBox* Body = WidgetTree->ConstructWidget<UHorizontalBox>();
	UVerticalBoxSlot* BodySlot = Box->AddChildToVerticalBox(Body);
	BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Scroll_Rows = WidgetTree->ConstructWidget<UScrollBox>();
	UHorizontalBoxSlot* ListSlot = Body->AddChildToHorizontalBox(Scroll_Rows);
	ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Text_Detail = Text(WidgetTree, 18);
	UHorizontalBoxSlot* DetailSlot = Body->AddChildToHorizontalBox(Text_Detail);
	DetailSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	DetailSlot->SetPadding(FMargin(24.f, 0.f, 0.f, 0.f));

	UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>();
	Box->AddChildToVerticalBox(Bottom)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	Button_Reset = Button(WidgetTree, ResetText);
	Bottom->AddChildToHorizontalBox(Button_Reset);
	Bar_ResetHold = WidgetTree->ConstructWidget<UProgressBar>();
	UHorizontalBoxSlot* BarSlot = Bottom->AddChildToHorizontalBox(Bar_ResetHold);
	BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	BarSlot->SetPadding(FMargin(12.f, 0.f));
	BarSlot->SetVerticalAlignment(VAlign_Center);
	Button_Close = Button(WidgetTree, CloseText);
	Bottom->AddChildToHorizontalBox(Button_Close);
}

void USoulHallOfFamePanelWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultTree();
	}
	if (Button_TabScore)
	{
		Button_TabScore->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleTabScore);
	}
	if (Button_TabTime)
	{
		Button_TabTime->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleTabTime);
	}
	if (Button_Close)
	{
		Button_Close->OnClicked.AddUniqueDynamic(this, &ThisClass::Close);
	}
	Rebuild();
}

void USoulHallOfFamePanelWidget::HandleTabScore()
{
	bSortByTime = false;
	Rebuild();
}

void USoulHallOfFamePanelWidget::HandleTabTime()
{
	bSortByTime = true;
	Rebuild();
}

FText USoulHallOfFamePanelWidget::FormatGrade(const FSoulHallOfFameEntry& Entry) const
{
	return Entry.RankId.IsNone() ? FailGradeText : FText::FromName(Entry.RankId);
}

void USoulHallOfFamePanelWidget::Rebuild()
{
	const USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this);
	Shown = HoF ? HoF->GetTopEntries(HoF->GetEntryCount()) : TArray<FSoulHallOfFameEntry>();
	if (bSortByTime)
	{
		// 시간 탭: 클리어한 기록만 빠른 순
		Shown.RemoveAll([](const FSoulHallOfFameEntry& E) { return !E.bSuccess; });
		Shown.StableSort([](const FSoulHallOfFameEntry& A, const FSoulHallOfFameEntry& B) { return A.ClearSeconds < B.ClearSeconds; });
	}
	if (!Scroll_Rows)
		return;
	Scroll_Rows->ClearChildren();
	if (Shown.IsEmpty())
	{
		UTextBlock* Empty = SoulTownUI::Text(WidgetTree, 18);
		Empty->SetText(EmptyText);
		Scroll_Rows->AddChild(Empty);
	}
	for (int32 i = 0; i < Shown.Num(); ++i)
	{
		const FSoulHallOfFameEntry& Entry = Shown[i];
		FFormatNamedArguments Args;
		Args.Add(TEXT("Rank"), FText::AsNumber(i + 1));
		Args.Add(TEXT("Name"), FText::FromString(Entry.PlayerName));
		Args.Add(TEXT("Score"), FText::AsNumber(Entry.Score));
		Args.Add(TEXT("Grade"), FormatGrade(Entry));
		Args.Add(TEXT("Time"), SoulTownUI::FormatTime(Entry.ClearSeconds));
		USoulHoFRowButton* Row = Cast<USoulHoFRowButton>(SoulTownUI::Button(WidgetTree, FText::Format(RowFormat, Args), USoulHoFRowButton::StaticClass()));
		Row->Bind(this, i);
		Scroll_Rows->AddChild(Row);
	}
	SelectEntry(Shown.IsEmpty() ? INDEX_NONE : 0);
}

void USoulHallOfFamePanelWidget::SelectEntry(int32 Index)
{
	if (!Text_Detail)
		return;
	if (!Shown.IsValidIndex(Index))
	{
		Text_Detail->SetText(FText::GetEmpty());
		return;
	}
	const FSoulHallOfFameEntry& Entry = Shown[Index];
	const USoulEquipmentCatalog* Catalog = USoulEquipmentCatalog::GetRunCatalog(this);
	const USoulCostumeDefinition* Costume = Catalog ? Catalog->FindCostumeByName(Entry.CostumeId.ToString()) : nullptr;
	const USoulEquipmentDefinition* Weapon = Catalog && !Entry.WeaponId.IsNone() ? Catalog->FindWeaponByName(Entry.WeaponId.ToString()) : nullptr;
	FFormatNamedArguments Args;
	Args.Add(TEXT("Name"), FText::FromString(Entry.PlayerName));
	Args.Add(TEXT("Score"), FText::AsNumber(Entry.Score));
	Args.Add(TEXT("Grade"), FormatGrade(Entry));
	Args.Add(TEXT("Time"), SoulTownUI::FormatTime(Entry.ClearSeconds));
	Args.Add(TEXT("Level"), FText::AsNumber(Entry.FinalLevel));
	Args.Add(TEXT("Kills"), FText::AsNumber(Entry.Stats.Kills));
	Args.Add(TEXT("Events"), FText::AsNumber(Entry.Stats.Events));
	Args.Add(TEXT("Damage"), FText::AsNumber(FMath::RoundToInt(Entry.Stats.DamageTaken)));
	Args.Add(TEXT("Costume"), Costume ? Costume->DisplayName : FText::FromName(Entry.CostumeId));
	Args.Add(TEXT("Weapon"), Weapon ? Weapon->DisplayName : FText::FromName(Entry.WeaponId));
	Args.Add(TEXT("Date"), FText::AsDate(Entry.DateUtc));
	Text_Detail->SetText(FText::Format(DetailFormat, Args));
}

void USoulHallOfFamePanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// 초기화: 버튼을 ResetHoldSec 동안 누르고 있어야 한다
	const bool bHolding = Button_Reset && Button_Reset->IsPressed() && ResetHoldSec > 0.f;
	ResetHeld = bHolding ? ResetHeld + InDeltaTime : 0.f;
	if (Bar_ResetHold)
	{
		Bar_ResetHold->SetPercent(ResetHoldSec > 0.f ? ResetHeld / ResetHoldSec : 0.f);
	}
	if (bHolding && ResetHeld >= ResetHoldSec)
	{
		ResetHeld = 0.f;
		if (USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this))
		{
			HoF->ResetRecords();
		}
		Rebuild();
	}
}

FReply USoulHallOfFamePanelWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::F)
	{
		Close();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
