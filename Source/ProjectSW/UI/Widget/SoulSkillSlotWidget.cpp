#include "UI/Widget/SoulSkillSlotWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GAS/Core/SoulAbilitySystemComponent.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Utility/SoulGameplayTags.h"

TWeakObjectPtr<USoulSkillSlotWidget> USoulSkillSlotWidget::PendingSwapSourceWidget = nullptr;

void USoulSkillSlotWidget::SetupSlot(FGameplayTag InSlotTag, int32 InFixedStepIndex, bool bInEnableChainEdit)
{
	SlotInputTag = InSlotTag;
	FixedStepIndex = InFixedStepIndex;
	bEnableChainEdit = bInEnableChainEdit;
	RefreshSlotUI();
}

void USoulSkillSlotWidget::ClearPendingSwapSelection()
{
	PendingSwapSourceWidget.Reset();
}

void USoulSkillSlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (Text_StepBadge)
	{
		const int32 SlotNum = GetSlotNumberFromTag();
		const int32 PreviewStep = FMath::Max(0, FixedStepIndex);
		Text_StepBadge->SetText(FText::FromString(FString::Printf(TEXT("%d-%d"), SlotNum, PreviewStep)));
	}
}

void USoulSkillSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Image_CooldownSweep)
	{
		if (CooldownMaterialBase)
		{
			CooldownMID = UMaterialInstanceDynamic::Create(CooldownMaterialBase, this);
			Image_CooldownSweep->SetBrushFromMaterial(CooldownMID);
		}
		else
		{
			CooldownMID = Image_CooldownSweep->GetDynamicMaterial();
		}
		Image_CooldownSweep->SetVisibility(ESlateVisibility::Hidden);
	}

	if (Text_Cooldown)
	{
		Text_Cooldown->SetVisibility(ESlateVisibility::Hidden);
	}

	if (ProgressBar_ChainTimer)
	{
		ProgressBar_ChainTimer->SetVisibility(ESlateVisibility::Hidden);
	}

	if (Image_SelectHighlight)
	{
		Image_SelectHighlight->SetVisibility(ESlateVisibility::Hidden);
	}

	if (USoulAbilitySystemComponent* SoulASC = GetSoulASC())
	{
		SoulASC->OnSkillChainStateChanged.RemoveDynamic(this, &ThisClass::HandleSkillChainStateChanged);
		SoulASC->OnSkillChainStateChanged.AddDynamic(this, &ThisClass::HandleSkillChainStateChanged);
	}

	RefreshSlotUI();
}

void USoulSkillSlotWidget::NativeDestruct()
{
	if (CachedSoulASC.IsValid())
	{
		CachedSoulASC->OnSkillChainStateChanged.RemoveDynamic(this, &ThisClass::HandleSkillChainStateChanged);
	}
	if (PendingSwapSourceWidget.Get() == this)
	{
		PendingSwapSourceWidget.Reset();
	}
	Super::NativeDestruct();
}

void USoulSkillSlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 아직 ASC가 바인딩되지 않았다면 재시도
	if (!CachedSoulASC.IsValid())
	{
		if (USoulAbilitySystemComponent* SoulASC = GetSoulASC())
		{
			SoulASC->OnSkillChainStateChanged.RemoveDynamic(this, &ThisClass::HandleSkillChainStateChanged);
			SoulASC->OnSkillChainStateChanged.AddDynamic(this, &ThisClass::HandleSkillChainStateChanged);
		}
	}

	RefreshSlotUI();
}

void USoulSkillSlotWidget::HandleSkillChainStateChanged()
{
	RefreshSlotUI();
}

void USoulSkillSlotWidget::RefreshSlotUI()
{
	if (Image_SelectHighlight)
	{
		const bool bIsSelectedForSwap = (PendingSwapSourceWidget.Get() == this);
		Image_SelectHighlight->SetVisibility(bIsSelectedForSwap ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	USoulAbilitySystemComponent* SoulASC = GetSoulASC();
	if (!SoulASC || !SlotInputTag.IsValid())
	{
		return;
	}

	FSoulSkillSlotUIData UIData;
	if (!SoulASC->GetSkillSlotUIData(SlotInputTag, FixedStepIndex, UIData))
	{
		return;
	}

	// 1. 스킬 아이콘 갱신 및 쿨타임 중 채도/밝기 조절
	if (Image_SkillIcon)
	{
		if (UIData.SkillIcon)
		{
			Image_SkillIcon->SetBrushFromTexture(UIData.SkillIcon, false);
			Image_SkillIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Image_SkillIcon->SetVisibility(ESlateVisibility::Hidden);
		}

		const bool bOnCooldown = (UIData.CooldownRemaining > 0.0f);
		Image_SkillIcon->SetColorAndOpacity(bOnCooldown
			? FLinearColor(0.35f, 0.35f, 0.35f, 1.0f)
			: FLinearColor::White);
	}

	// 2. 체인 단계 배지 텍스트 갱신 ("1-0", "1-1", "1-2")
	if (Text_StepBadge)
	{
		const int32 SlotNum = GetSlotNumberFromTag();
		Text_StepBadge->SetText(FText::FromString(FString::Printf(TEXT("%d-%d"), SlotNum, UIData.DisplayStep)));
	}

	// 3. 시계방향 쿨타임 오버레이 및 남은 시간 숫자 갱신
	const bool bIsCoolingDown = (UIData.CooldownRemaining > 0.0f && UIData.CooldownDuration > 0.0f);
	if (Image_CooldownSweep)
	{
		if (bIsCoolingDown)
		{
			const float Ratio = FMath::Clamp(UIData.CooldownRemaining / UIData.CooldownDuration, 0.0f, 1.0f);
			if (CooldownMID)
			{
				CooldownMID->SetScalarParameterValue(CooldownPercentParamName, Ratio);
			}
			Image_CooldownSweep->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Image_CooldownSweep->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (Text_Cooldown)
	{
		if (bIsCoolingDown)
		{
			const FString CooldownStr = (UIData.CooldownRemaining >= 10.0f)
				? FString::Printf(TEXT("%d"), FMath::CeilToInt(UIData.CooldownRemaining))
				: FString::Printf(TEXT("%.1f"), UIData.CooldownRemaining);
			Text_Cooldown->SetText(FText::FromString(CooldownStr));
			Text_Cooldown->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text_Cooldown->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 4. 3초 체인 유예 타이머 바 갱신 (HUD 모드에서만 표시)
	if (ProgressBar_ChainTimer)
	{
		if (FixedStepIndex < 0 && UIData.bIsChainWindowActive)
		{
			ProgressBar_ChainTimer->SetPercent(UIData.ChainWindowRemainingRatio);
			ProgressBar_ChainTimer->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			ProgressBar_ChainTimer->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

FReply USoulSkillSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bEnableChainEdit && FixedStepIndex >= 0 && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// 이미 다른 슬롯이 클릭 선택된 상태라면 즉시 두 스킬의 체인 위치를 교체 (Click-to-Swap 지원)
		if (PendingSwapSourceWidget.IsValid())
		{
			USoulSkillSlotWidget* SourceWidget = PendingSwapSourceWidget.Get();
			PendingSwapSourceWidget.Reset();

			if (SourceWidget != this)
			{
				if (USoulAbilitySystemComponent* SoulASC = GetSoulASC())
				{
					SoulASC->SwapSkillChainSteps(
						SourceWidget->SlotInputTag, SourceWidget->FixedStepIndex,
						SlotInputTag, FixedStepIndex);
				}
				SourceWidget->RefreshSlotUI();
				RefreshSlotUI();
				return FReply::Handled();
			}
		}
		else
		{
			PendingSwapSourceWidget = this;
			RefreshSlotUI();
		}

		// 마우스를 누른 채 이동하면 드래그 앤 드롭 시작
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void USoulSkillSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (!bEnableChainEdit || FixedStepIndex < 0)
	{
		return;
	}

	// 드래그 시작 시 클릭 선택 상태는 해제
	PendingSwapSourceWidget.Reset();

	USoulSkillDragDropOperation* DragOp = NewObject<USoulSkillDragDropOperation>(this);
	DragOp->SourceSlotTag = SlotInputTag;
	DragOp->SourceStepIndex = FixedStepIndex;
	DragOp->Pivot = EDragPivot::CenterCenter;

	// 드래그 중 마우스 커서를 따라다닐 반투명 아이콘 생성
	if (Image_SkillIcon)
	{
		UImage* DragVisual = NewObject<UImage>(this);
		DragVisual->SetBrush(Image_SkillIcon->GetBrush());
		DragVisual->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.85f));
		DragOp->DefaultDragVisual = DragVisual;
	}

	OutOperation = DragOp;
}

bool USoulSkillSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const USoulSkillDragDropOperation* SkillDragOp = Cast<USoulSkillDragDropOperation>(InOperation);
	if (!bEnableChainEdit || FixedStepIndex < 0 || !SkillDragOp)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	PendingSwapSourceWidget.Reset();

	if (USoulAbilitySystemComponent* SoulASC = GetSoulASC())
	{
		return SoulASC->SwapSkillChainSteps(
			SkillDragOp->SourceSlotTag, SkillDragOp->SourceStepIndex,
			SlotInputTag, FixedStepIndex);
	}

	return false;
}

USoulAbilitySystemComponent* USoulSkillSlotWidget::GetSoulASC()
{
	if (CachedSoulASC.IsValid())
	{
		return CachedSoulASC.Get();
	}

	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwningPawn))
		{
			CachedSoulASC = Cast<USoulAbilitySystemComponent>(ASC);
		}
	}

	return CachedSoulASC.Get();
}

int32 USoulSkillSlotWidget::GetSlotNumberFromTag() const
{
	if (SlotInputTag.MatchesTagExact(SoulTags::InputTags::KEY_1)) return 1;
	if (SlotInputTag.MatchesTagExact(SoulTags::InputTags::KEY_2)) return 2;
	if (SlotInputTag.MatchesTagExact(SoulTags::InputTags::KEY_3)) return 3;
	if (SlotInputTag.MatchesTagExact(SoulTags::InputTags::KEY_4)) return 4;
	return 1;
}