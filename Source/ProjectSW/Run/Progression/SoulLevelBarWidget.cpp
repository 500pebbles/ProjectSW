#include "Run/Progression/SoulLevelBarWidget.h"
#include "Run/Progression/SoulProgressionComponent.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void USoulLevelBarWidget::Bind(USoulProgressionComponent* InProgression)
{
	if (!InProgression)
		return;
	InProgression->OnProgressionChanged.AddUniqueDynamic(this, &ThisClass::HandleChanged);
	HandleChanged(InProgression->GetLevel(), InProgression->GetExp(), InProgression->GetExpToNext());
}

void USoulLevelBarWidget::HandleChanged(int32 Level, int32 Exp, int32 ExpToNext)
{
	Refresh(Level, Exp, ExpToNext);
}

void USoulLevelBarWidget::Refresh_Implementation(int32 Level, int32 Exp, int32 ExpToNext)
{
	if (Bar_Exp)
	{
		Bar_Exp->SetPercent(ExpToNext > 0 ? static_cast<float>(Exp) / ExpToNext : 1.f);
	}
	if (Text_Level)
	{
		Text_Level->SetText(FText::FromString(FString::Printf(TEXT("Lv.%d"), Level)));
	}
}
