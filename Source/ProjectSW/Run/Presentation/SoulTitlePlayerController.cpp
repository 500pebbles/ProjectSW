#include "Run/Presentation/SoulTitlePlayerController.h"
#include "Run/Presentation/SoulTransitionSubsystem.h"
#include "Run/Core/SoulShots.h"
#include "Run/UI/SoulTitleWidget.h"
#include "Utility/SoulLog.h"

#include "Engine/World.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void ASoulTitlePlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
		return;
	if (TitleWidgetClass)
	{
		TitleWidget = CreateWidget<USoulTitleWidget>(this, TitleWidgetClass);
		if (TitleWidget)
		{
			TitleWidget->AddToViewport();
		}
	}
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
	ShownTime = FPlatformTime::Seconds();
	SW_LOG("[TITLE] 시작 위젯=%d", TitleWidget != nullptr);

#if !UE_BUILD_SHIPPING
	// 자동 확인: -SoulTitleAutoKey면 잠시 뒤 아무 키를 누른 것으로 친다
	if (FParse::Param(FCommandLine::Get(), TEXT("SoulTitleAutoKey")))
	{
		SoulShots::Take(TEXT("01_title"), MinInputDelay + 0.6f);
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			FInputKeyEventArgs Args = FInputKeyEventArgs::CreateSimulated(EKeys::SpaceBar, IE_Pressed, 1.f);
			InputKey(Args);
		}), MinInputDelay + 1.f, false);
	}
#endif
}

bool ASoulTitlePlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	const bool bHandled = Super::InputKey(Params);
	if (bStarted || FPlatformTime::Seconds() - ShownTime < MinInputDelay)
		return bHandled;

	const FKey& Key = Params.Key;
	if (IgnoredKeys.Contains(Key))
		return bHandled;

	bool bAccept = false;
	if (Key.IsAnalog())
	{
		// 패드 스틱과 트리거만. 마우스 이동(MouseX/Y)은 무시
		bAccept = Key.IsGamepadKey() && FMath::Abs(Params.AmountDepressed) >= StickThreshold;
	}
	else
	{
		bAccept = Params.Event == IE_Pressed;
	}
	if (!bAccept)
		return bHandled;

	bStarted = true;
	OnAnyKey(Key);
	return true;
}

void ASoulTitlePlayerController::OnAnyKey_Implementation(FKey Key)
{
	SW_LOG("[TITLE] key %s", *Key.ToString());
	UGameplayStatics::PlaySound2D(this, StartSound);
	if (TitleWidget)
	{
		TitleWidget->PlayOut(OutTime);
	}
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (USoulTransitionSubsystem* Transition = USoulTransitionSubsystem::Get(this))
		{
			Transition->TravelToTown(this);
		}
	}), FMath::Max(OutTime, 0.01f), false);
}
