#include "Run/UI/SoulResultWidget.h"
#include "Run/Core/SoulRunConfig.h"
#include "Run/Core/SoulRunGameComponent.h"
#include "Run/Equipment/SoulCostumeDefinition.h"
#include "Run/Equipment/SoulEquipmentCatalog.h"
#include "Run/Equipment/SoulEquipmentDefinition.h"
#include "Run/Presentation/SoulTransitionSubsystem.h"
#include "Run/Save/SoulHallOfFameSubsystem.h"
#include "Run/Save/SoulScoreRules.h"
#include "Utility/SoulLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"

namespace SoulResult
{
	FString FormatTime(float Seconds)
	{
		const int32 Total = FMath::Max(0, FMath::RoundToInt(Seconds));
		return FString::Printf(TEXT("%02d:%02d"), Total / 60, Total % 60);
	}
}

USoulResultWidget* USoulResultWidget::ShowResult(APlayerController* PC, TSubclassOf<USoulResultWidget> WidgetClass, const FSoulRunSummary& Summary, FGuid EntryId)
{
	if (!PC || !WidgetClass)
		return nullptr;
	USoulResultWidget* Widget = CreateWidget<USoulResultWidget>(PC, WidgetClass);
	if (!Widget)
		return nullptr;
	Widget->Setup(Summary, EntryId);
	Widget->AddToViewport(100);
	// 표시 순서: 눌린 키 비우기 → UI 입력(이름 칸이나 귀환 버튼에 포커스) → 커서
	PC->FlushPressedKeys();
	FInputModeUIOnly Mode;
	UWidget* Focus = Widget->NameInput && Widget->NameRow && Widget->NameRow->IsVisible() ? static_cast<UWidget*>(Widget->NameInput) : static_cast<UWidget*>(Widget->ReturnButton);
	if (Focus)
	{
		Mode.SetWidgetToFocus(Focus->TakeWidget());
	}
	PC->SetInputMode(Mode);
	PC->SetShowMouseCursor(true);
	SW_LOG("[RESULT] show success=%d score=%d rank=%s entry=%d", Summary.bSuccess, Summary.Score, *Summary.RankId.ToString(), EntryId.IsValid());
	return Widget;
}

UTextBlock* USoulResultWidget::MakeText(UVerticalBox* Box, int32 Size, const FLinearColor& Color)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = Size;
	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* BoxSlot = Box->AddChildToVerticalBox(Text))
	{
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
		BoxSlot->SetPadding(FMargin(0.f, 6.f));
	}
	return Text;
}

void USoulResultWidget::BuildDefaultTree()
{
	// WBP 트리가 비어 있을 때만 기본 배치 (사람이 WBP에서 바꾸면 이 경로를 안 탄다)
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Root"));
	Root->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.72f));
	Root->SetHorizontalAlignment(HAlign_Center);
	Root->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Root;

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	Root->SetContent(Box);
	BannerText = MakeText(Box, 48, FLinearColor(1.f, 0.85f, 0.4f));
	RankText = MakeText(Box, 96, FLinearColor::White);
	ScoreText = MakeText(Box, 40, FLinearColor::White);
	BreakdownText = MakeText(Box, 18, FLinearColor(0.85f, 0.85f, 0.85f));
	DetailText = MakeText(Box, 20, FLinearColor(0.9f, 0.9f, 0.9f));

	// 사용한 의상과 무기 아이콘 (#8)
	UHorizontalBox* IconRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	if (UVerticalBoxSlot* BoxSlot = Box->AddChildToVerticalBox(IconRow))
	{
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
	}
	for (TObjectPtr<UImage>* Icon : { &Image_Costume, &Image_Weapon })
	{
		*Icon = WidgetTree->ConstructWidget<UImage>();
		(*Icon)->SetDesiredSizeOverride(FVector2D(64.f, 64.f));
		IconRow->AddChildToHorizontalBox(*Icon)->SetPadding(FMargin(8.f, 0.f));
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	NameRow = Row;
	if (UVerticalBoxSlot* BoxSlot = Box->AddChildToVerticalBox(Row))
	{
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
		BoxSlot->SetPadding(FMargin(0.f, 12.f));
	}
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetText(NameLabel);
	Row->AddChildToHorizontalBox(Label)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	NameInput = WidgetTree->ConstructWidget<UEditableText>();
	NameInput->SetMinimumDesiredWidth(260.f);
	Row->AddChildToHorizontalBox(NameInput);
	NameCountText = WidgetTree->ConstructWidget<UTextBlock>();
	Row->AddChildToHorizontalBox(NameCountText)->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));

	ReturnButton = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>();
	ButtonText->SetText(ReturnLabel);
	ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	ReturnButton->SetContent(ButtonText);
	if (UVerticalBoxSlot* BoxSlot = Box->AddChildToVerticalBox(ReturnButton))
	{
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
		BoxSlot->SetPadding(FMargin(0.f, 12.f));
	}
}

void USoulResultWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultTree();
	}
	if (ReturnButton)
	{
		ReturnButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleReturnClicked);
	}
	if (NameInput)
	{
		// 마지막 이름이 선택된 상태라 새로 치면 바뀌고 Enter만 쳐도 된다 (WBP로 다시 꾸며도 강제)
		NameInput->SetSelectAllTextWhenFocused(true);
		NameInput->OnTextChanged.AddUniqueDynamic(this, &ThisClass::HandleNameChanged);
		NameInput->OnTextCommitted.AddUniqueDynamic(this, &ThisClass::HandleNameCommitted);
	}
}

void USoulResultWidget::Setup(const FSoulRunSummary& InSummary, FGuid InEntryId)
{
	Summary = InSummary;
	EntryId = InEntryId;
	CountUpElapsed = 0.f;

	const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(this);
	const USoulEquipmentCatalog* Catalog = Config ? Config->EquipmentCatalog.Get() : nullptr;

	if (BannerText)
	{
		BannerText->SetText(Summary.bSuccess ? ClearBanner : FailBanner);
	}
	if (RankText)
	{
		RankText->SetText(FText::FromName(Summary.RankId.IsNone() ? FName(TEXT("-")) : Summary.RankId));
		if (Config && Config->ScoreRules)
		{
			RankText->SetColorAndOpacity(FSlateColor(Config->ScoreRules->GetRankColor(Summary.RankId)));
		}
		RankText->SetVisibility(Summary.bSuccess ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (BreakdownText)
	{
		FString Lines;
		for (const FSoulScoreLine& Line : Summary.Breakdown)
		{
			const FText* Label = CategoryLabels.Find(Line.Category);
			Lines += FString::Printf(TEXT("%s   %s\n"), Label ? *Label->ToString() : *Line.Category.ToString(), *FText::AsNumber(Line.Score).ToString());
		}
		BreakdownText->SetText(FText::FromString(Lines.TrimEnd()));
	}
	if (DetailText)
	{
		const USoulCostumeDefinition* Costume = Catalog ? Catalog->FindCostumeByName(Summary.CostumeId.ToString()) : nullptr;
		const USoulEquipmentDefinition* Weapon = Catalog ? Catalog->FindWeaponByName(Summary.WeaponId.ToString()) : nullptr;
		FFormatNamedArguments Args;
		Args.Add(TEXT("Time"), FText::FromString(SoulResult::FormatTime(Summary.ClearSeconds)));
		Args.Add(TEXT("Level"), FText::AsNumber(Summary.FinalLevel));
		Args.Add(TEXT("Costume"), Costume ? Costume->DisplayName : FText::FromName(Summary.CostumeId));
		Args.Add(TEXT("Weapon"), Weapon ? Weapon->DisplayName : FText::FromName(Summary.WeaponId));
		Args.Add(TEXT("Kills"), FText::AsNumber(Summary.Stats.Kills));
		Args.Add(TEXT("Events"), FText::AsNumber(Summary.Stats.Events));
		DetailText->SetText(FText::Format(DetailFormat, Args));
	}
	UTexture2D* CostumeIcon = Catalog ? Catalog->FindCostumeIcon(Summary.CostumeId) : nullptr;
	UTexture2D* WeaponIcon = Catalog ? Catalog->FindWeaponIcon(Summary.WeaponId) : nullptr;
	USoulEquipmentCatalog::SetIcon(Image_Costume, CostumeIcon);
	USoulEquipmentCatalog::SetIcon(Image_Weapon, WeaponIcon);
	SW_LOG("[RESULT] icons costume=%s weapon=%s", CostumeIcon ? *CostumeIcon->GetName() : TEXT("none"), WeaponIcon ? *WeaponIcon->GetName() : TEXT("none"));
	const bool bNameEntry = EntryId.IsValid();
	if (NameRow)
	{
		NameRow->SetVisibility(bNameEntry ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (NameInput && bNameEntry)
	{
		const USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this);
		NameInput->SetText(FText::FromString(HoF ? HoF->GetLastPlayerName() : FString()));
		HandleNameChanged(NameInput->GetText());
	}
	if (ScoreText)
	{
		ScoreText->SetText(FText::AsNumber(CountUpTime > 0.f ? 0 : Summary.Score));
	}
}

void USoulResultWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (ScoreText && CountUpTime > 0.f && CountUpElapsed < CountUpTime)
	{
		CountUpElapsed = FMath::Min(CountUpElapsed + InDeltaTime, CountUpTime);
		const float Alpha = FMath::InterpEaseOut(0.f, 1.f, CountUpElapsed / CountUpTime, 3.f);
		ScoreText->SetText(FText::AsNumber(FMath::RoundToInt(Summary.Score * Alpha)));
	}
}

void USoulResultWidget::HandleNameChanged(const FText& Text)
{
	// 조합 중 자르면 한글 조합이 깨지므로 입력 중에는 글자 수만 보인다. 자르기는 커밋 때
	if (NameCountText)
	{
		const USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this);
		NameCountText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), Text.ToString().TrimStartAndEnd().Len(), HoF ? HoF->GetNameMaxLength() : 0)));
	}
}

void USoulResultWidget::HandleNameCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		ReturnToTown();
	}
}

void USoulResultWidget::SetNameForTest(const FString& Name)
{
	if (NameInput)
	{
		NameInput->SetText(FText::FromString(Name));
		HandleNameChanged(NameInput->GetText());
	}
}

void USoulResultWidget::HandleReturnClicked()
{
	ReturnToTown();
}

void USoulResultWidget::ReturnToTown()
{
	if (bReturning)
		return;
	bReturning = true;
	if (EntryId.IsValid() && NameInput)
	{
		if (USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this))
		{
			HoF->SetEntryName(EntryId, NameInput->GetText().ToString());
		}
	}
	if (USoulTransitionSubsystem* Transition = USoulTransitionSubsystem::Get(this))
	{
		Transition->TravelToTown(this);
	}
}
