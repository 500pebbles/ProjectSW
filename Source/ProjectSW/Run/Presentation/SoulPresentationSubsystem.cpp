#include "Run/Presentation/SoulPresentationSubsystem.h"
#include "Run/Presentation/SoulPresentationData.h"
#include "Run/Presentation/SoulTransitionSubsystem.h"
#include "Run/Core/SoulRunConfig.h"
#include "Run/Core/SoulRunGameComponent.h"
#include "Run/UI/SoulPresentationWidgets.h"
#include "Run/UI/SoulKeyHintWidget.h"
#include "Run/Progression/SoulLevelBarWidget.h"
#include "UI/Widget/SoulUserWidget.h"
#include "Inventory/UI/InventoryHUDWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Character/SoulCharacterEnemy.h"
#include "Player/SoulPlayerController.h"
#include "Components/AudioComponent.h"
#include "Utility/SoulLog.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "BrainComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

const FSoulLoadingEntry* USoulPresentationData::FindLoading(FName Id) const
{
	return LoadingEntries.FindByPredicate([Id](const FSoulLoadingEntry& Entry) { return Entry.Id == Id; });
}

const FSoulBossIntro* USoulPresentationData::FindBossIntro(const UObject* Boss) const
{
	if (!Boss)
		return nullptr;
	FString Key = Boss->GetClass()->GetName();
	Key.RemoveFromEnd(TEXT("_C"));
	return BossIntros.Find(FName(*Key));
}

USoulPresentationSubsystem* USoulPresentationSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<USoulPresentationSubsystem>() : nullptr;
}

TStatId USoulPresentationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USoulPresentationSubsystem, STATGROUP_Tickables);
}

const USoulPresentationData* USoulPresentationSubsystem::GetData() const
{
	const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(GetWorld());
	return Config ? Config->Presentation.Get() : nullptr;
}

void USoulPresentationSubsystem::ShowBanner(const FText& Title, const FText& Subtitle, const FLinearColor& Color) const
{
	const USoulPresentationData* Data = GetData();
	if (Data)
	{
		USoulBannerWidget::ShowBanner(GetWorld()->GetFirstPlayerController(), Data->BannerWidgetClass, Title, Subtitle, Color, Data->BannerHoldSec);
	}
}

void USoulPresentationSubsystem::PlaySound2D(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), Sound);
	}
}

void USoulPresentationSubsystem::PlayAt(USoundBase* Sound, UNiagaraSystem* FX, const FVector& Location) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, Location);
	}
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), FX, Location);
}

// ---------- 보스 인트로 ----------

void USoulPresentationSubsystem::SetIntroStage(ESoulBossIntroStage Stage)
{
	SW_LOG("[PRES] boss intro stage=%d boss=%s", static_cast<int32>(Stage), IntroBoss.IsValid() ? *IntroBoss->GetName() : TEXT("none"));
	OnIntroStage.Broadcast(Stage);
}

void USoulPresentationSubsystem::PlayBgm(USoundBase* Sound)
{
	if (!Sound)
	{
		StopBgm();
		return;
	}
	if (Sound == BgmSound && IsValid(BgmComponent) && BgmComponent->IsPlaying())
		return;

	const USoulPresentationData* Data = GetData();
	StopBgm();
	BgmComponent = UGameplayStatics::CreateSound2D(GetWorld(), Sound, Data ? Data->BgmVolume : 0.5f, 1.f, 0.f, nullptr, false, true);
	if (BgmComponent)
	{
		BgmComponent->FadeIn(Data ? Data->BgmFadeInSec : 1.f, 1.f);
		BgmSound = Sound;
	}
	SW_LOG("[PRES] bgm %s", *Sound->GetName());
}

void USoulPresentationSubsystem::StopBgm(float FadeOutSec)
{
	if (IsValid(BgmComponent))
	{
		const USoulPresentationData* Data = GetData();
		const float Fade = FadeOutSec >= 0.f ? FadeOutSec : (Data ? Data->BgmFadeOutSec : 1.f);
		if (Fade > 0.f)
		{
			BgmComponent->FadeOut(Fade, 0.f);
		}
		else
		{
			BgmComponent->Stop();
		}
		SW_LOG("[PRES] bgm stop %s", BgmSound ? *BgmSound->GetName() : TEXT("?"));
	}
	BgmComponent = nullptr;
	BgmSound = nullptr;
}

void USoulPresentationSubsystem::SetGameHudHidden(FName Reason, bool bHidden)
{
	const bool bWasHidden = !HudHideReasons.IsEmpty();
	if (bHidden)
	{
		HudHideReasons.Add(Reason);
	}
	else
	{
		HudHideReasons.Remove(Reason);
	}
	const bool bNowHidden = !HudHideReasons.IsEmpty();
	if (bWasHidden == bNowHidden)
		return;

	if (bNowHidden)
	{
		TArray<UUserWidget*> Widgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UUserWidget::StaticClass(), true);
		for (UUserWidget* Widget : Widgets)
		{
			const bool bHud = Widget->IsA<USoulUserWidget>() || Widget->IsA<UInventoryHUDWidget>() || Widget->IsA<USoulKeyHintWidget>() || Widget->IsA<USoulLevelBarWidget>();
			if (bHud && Widget->IsInViewport() && Widget->GetVisibility() != ESlateVisibility::Collapsed && Widget->GetVisibility() != ESlateVisibility::Hidden)
			{
				HiddenHudWidgets.Emplace(Widget, Widget->GetVisibility());
				Widget->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
	else
	{
		for (const TPair<TWeakObjectPtr<UUserWidget>, ESlateVisibility>& Entry : HiddenHudWidgets)
		{
			if (UUserWidget* Widget = Entry.Key.Get())
			{
				Widget->SetVisibility(Entry.Value);
			}
		}
	}
	SW_LOG("[PRES] hud hidden=%d reason=%s widgets=%d", bNowHidden, *Reason.ToString(), HiddenHudWidgets.Num());
	if (!bNowHidden)
	{
		HiddenHudWidgets.Reset();
	}
}

void USoulPresentationSubsystem::PlayBossIntro(ASoulCharacterEnemy* Boss)
{
	const USoulPresentationData* Data = GetData();
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	// 같은 보스로 다시 들어오면(문 쪽으로 물러났다 재진입) 무시한다
	if (!Data || !Boss || !PC || bIntroPlaying || IntroBoss.Get() == Boss)
		return;

	IntroBoss = Boss;
	StopBgm();
	if (!Boss->OnDied.IsBoundToObject(this))
	{
		Boss->OnDied.AddUObject(this, &ThisClass::HandleBossDied);
	}
	const FSoulBossIntro* Intro = Data->FindBossIntro(Boss);
	if (!Intro)
	{
		SW_LOG_WARNING("[PRES] %s 보스 인트로 값이 없음 (DA_Presentation BossIntros). 체력 바만 띄운다", *Boss->GetClass()->GetName());
	}
	if (!bWarnedSkipKeys && (Data->BossIntroSkipKeys.IsEmpty() || Data->BossIntroSkipHoldSec <= 0.f))
	{
		bWarnedSkipKeys = true;
		SW_LOG_WARNING("[PRES] 인트로 스킵 키나 누르는 시간이 비어 스킵이 꺼져 있음 (DA_Presentation)");
	}

	// 이전 보스 HP 바가 남아 있으면 지운다
	if (BossWidget)
	{
		BossWidget->RemoveFromParent();
		BossWidget = nullptr;
	}
	if (Data->BossIntroWidgetClass)
	{
		BossWidget = CreateWidget<USoulBossIntroWidget>(PC, Data->BossIntroWidgetClass);
		if (BossWidget)
		{
			BossWidget->AddToViewport(40);
			BossWidget->BindBoss(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Boss), Intro ? Intro->Name : FText::FromString(Boss->GetName()));
		}
	}
	if (!Intro)
	{
		// 인트로 값이 없으면 체력 바만
		if (BossWidget)
		{
			BossWidget->ShowFight();
		}
		return;
	}

	bIntroPlaying = true;
	SetGameHudHidden(TEXT("Intro"), true);
	bReturning = false;
	bSkipped = false;
	IntroSounds.Reset();
	USoulTransitionSubsystem::PrepareForTravel(PC);
	// FlushPressedKeys가 만든 Released(질주 해제 등)가 돈 다음 프레임에 입력을 끈다 (Director::DisableInputAfterFrame과 같은 방식)
	DisableInputAfterFrame(PC, GFrameCounter);
	if (APawn* Pawn = PC->GetPawn())
	{
		// 보스를 플레이어 쪽으로
		const FVector ToPlayer = (Pawn->GetActorLocation() - Boss->GetActorLocation()).GetSafeNormal2D();
		Boss->SetActorRotation(ToPlayer.Rotation());
	}
	if (const AAIController* AI = Cast<AAIController>(Boss->GetController()); AI && AI->GetBrainComponent())
	{
		AI->GetBrainComponent()->PauseLogic(TEXT("BossIntro"));
	}
	if (BossWidget)
	{
		BossWidget->ShowIntro(Data->WarningText, Data->BossIntroSkipHoldSec > 0.f && !Data->BossIntroSkipKeys.IsEmpty() ? Data->BossIntroSkipHint : FText::GetEmpty());
	}
	PlayIntroSound2D(Data->BossDoorSound);
	PlayIntroSound2D(Data->WarningSound);
	SetIntroStage(ESoulBossIntroStage::Start);
	GetWorld()->GetTimerManager().SetTimer(IntroTimer, this, &ThisClass::IntroCameraStage, FMath::Max(Data->BossIntroLeadSec, 0.01f), false);
}

void USoulPresentationSubsystem::PlayIntroSound2D(USoundBase* Sound)
{
	if (Sound)
	{
		if (UAudioComponent* Audio = UGameplayStatics::SpawnSound2D(GetWorld(), Sound))
		{
			IntroSounds.Add(Audio);
		}
	}
}

void USoulPresentationSubsystem::DisableInputAfterFrame(TWeakObjectPtr<APlayerController> WeakPC, uint64 LockFrame)
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, WeakPC, LockFrame]()
	{
		APlayerController* LockedPC = WeakPC.Get();
		// 그새 인트로가 끝났거나 스킵 복귀 중이면 끄지 않는다
		if (!LockedPC || !bIntroPlaying || bReturning)
			return;
		if (GFrameCounter <= LockFrame + 1)
		{
			DisableInputAfterFrame(WeakPC, LockFrame);
			return;
		}
		LockedPC->DisableInput(LockedPC);
		if (APawn* Pawn = LockedPC->GetPawn())
		{
			Pawn->DisableInput(LockedPC);
		}
	}));
}

void USoulPresentationSubsystem::IntroCameraStage()
{
	const USoulPresentationData* Data = GetData();
	ASoulCharacterEnemy* Boss = IntroBoss.Get();
	const FSoulBossIntro* Intro = Data ? Data->FindBossIntro(Boss) : nullptr;
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!Intro || !Boss || !PC)
	{
		FinishIntro();
		return;
	}
	const FVector Target = Boss->GetActorLocation() + FVector(0.f, 0.f, Intro->CameraHeight * 0.5f);
	const FVector CameraLocation = Boss->GetActorLocation() + Boss->GetActorForwardVector() * Intro->CameraDistance + FVector(0.f, 0.f, Intro->CameraHeight);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	IntroCameraActor = GetWorld()->SpawnActor<ACameraActor>(CameraLocation, (Target - CameraLocation).Rotation(), Params);
	if (IntroCameraActor)
	{
		IntroCameraActor->GetCameraComponent()->bConstrainAspectRatio = false;
		PC->SetViewTargetWithBlend(IntroCameraActor, Data->BossIntroBlendSec, VTBlend_EaseInOut, 2.f);
	}
	if (BossWidget)
	{
		const FText Name = Intro->Name;
		const FText Title = Intro->Title;
		if (Data->BossIntroNameCardDelaySec > 0.f)
		{
			GetWorld()->GetTimerManager().SetTimer(NameCardTimer, FTimerDelegate::CreateWeakLambda(this, [this, Name, Title]()
			{
				if (BossWidget && bIntroPlaying)
				{
					BossWidget->ShowNameCard(Name, Title);
				}
			}), Data->BossIntroNameCardDelaySec, false);
		}
		else
		{
			BossWidget->ShowNameCard(Name, Title);
		}
	}
	SetIntroStage(ESoulBossIntroStage::Camera);
	GetWorld()->GetTimerManager().SetTimer(IntroTimer, this, &ThisClass::IntroClip, FMath::Max(Data->BossIntroBlendSec, 0.01f), false);
}

void USoulPresentationSubsystem::IntroClip()
{
	const USoulPresentationData* Data = GetData();
	ASoulCharacterEnemy* Boss = IntroBoss.Get();
	const FSoulBossIntro* Intro = Data ? Data->FindBossIntro(Boss) : nullptr;
	if (!Intro || !Boss)
	{
		IntroReturn();
		return;
	}
	float Length = 0.f;
	if (Intro->Clip)
	{
		const float End = Intro->ClipEnd > 0.f ? Intro->ClipEnd : Intro->Clip->GetPlayLength();
		Length = FMath::Max(End - Intro->ClipStart, 0.1f);
		if (UAnimInstance* Anim = Boss->GetMesh() ? Boss->GetMesh()->GetAnimInstance() : nullptr)
		{
			Anim->PlaySlotAnimationAsDynamicMontage(Intro->Clip, TEXT("DefaultSlot"), Data->BossIntroClipBlendIn, Data->BossIntroClipBlendOut, 1.f, 1, -1.f, Intro->ClipStart);
			TWeakObjectPtr<UAnimInstance> WeakAnim = Anim;
			GetWorld()->GetTimerManager().SetTimer(ClipStopTimer, FTimerDelegate::CreateWeakLambda(this, [WeakAnim]()
			{
				if (WeakAnim.IsValid())
				{
					WeakAnim->Montage_Stop(0.2f);
				}
			}), Length, false);
		}
	}
	if (Intro->Sound)
	{
		if (UAudioComponent* Audio = UGameplayStatics::SpawnSoundAtLocation(GetWorld(), Intro->Sound, Boss->GetActorLocation()))
		{
			IntroSounds.Add(Audio);
		}
	}
	PlayAt(nullptr, Intro->FX, Boss->GetActorLocation());
	SetIntroStage(ESoulBossIntroStage::Clip);
	GetWorld()->GetTimerManager().SetTimer(IntroTimer, this, &ThisClass::IntroReturn, FMath::Max(Length + Data->BossIntroHoldSec, 0.01f), false);
}

void USoulPresentationSubsystem::IntroReturn()
{
	const USoulPresentationData* Data = GetData();
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	bReturning = true;
	const float Blend = Data ? (bSkipped ? Data->BossIntroSkipBlendSec : Data->BossIntroBlendSec) : 0.f;
	if (PC && PC->GetPawn())
	{
		PC->SetViewTargetWithBlend(PC->GetPawn(), Blend, VTBlend_EaseInOut, 2.f);
	}
	if (!bSkipped)
	{
		SetIntroStage(ESoulBossIntroStage::Return);
	}
	GetWorld()->GetTimerManager().SetTimer(IntroTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { FinishIntro(true); }), FMath::Max(Blend, 0.01f), false);
}

void USoulPresentationSubsystem::SkipIntro()
{
	if (!bIntroPlaying || bReturning || bSkipped)
		return;
	bSkipped = true;
	GetWorld()->GetTimerManager().ClearTimer(IntroTimer);
	GetWorld()->GetTimerManager().ClearTimer(ClipStopTimer);
	GetWorld()->GetTimerManager().ClearTimer(NameCardTimer);
	const USoulPresentationData* Data = GetData();
	// 인트로 소리를 줄인다
	for (UAudioComponent* Audio : IntroSounds)
	{
		if (IsValid(Audio) && Audio->IsPlaying())
		{
			Audio->FadeOut(Data ? Data->BossIntroSkipSoundFadeSec : 0.3f, 0.f);
		}
	}
	IntroSounds.Reset();
	if (ASoulCharacterEnemy* Boss = IntroBoss.Get())
	{
		if (UAnimInstance* Anim = Boss->GetMesh() ? Boss->GetMesh()->GetAnimInstance() : nullptr)
		{
			Anim->Montage_Stop(Data ? Data->BossIntroSkipBlendSec : 0.2f);
		}
	}
	SetIntroStage(ESoulBossIntroStage::Skipped);
	IntroReturn();
}

void USoulPresentationSubsystem::FinishIntro(bool bRestoreInput)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	GetWorld()->GetTimerManager().ClearTimer(NameCardTimer);
	IntroSounds.Reset();
	if (PC && bRestoreInput)
	{
		PC->EnableInput(PC);
		if (APawn* Pawn = PC->GetPawn())
		{
			Pawn->EnableInput(PC);
		}
		// 잠금 중 버려진 대시 해제를 다시 맞춘다
		if (ASoulPlayerController* SoulPC = Cast<ASoulPlayerController>(PC))
		{
			SoulPC->RefreshMoveSpeed();
		}
	}
	if (const ASoulCharacterEnemy* Boss = IntroBoss.Get())
	{
		if (const AAIController* AI = Cast<AAIController>(Boss->GetController()); AI && AI->GetBrainComponent())
		{
			AI->GetBrainComponent()->ResumeLogic(TEXT("BossIntro"));
		}
	}
	if (IntroCameraActor)
	{
		IntroCameraActor->Destroy();
		IntroCameraActor = nullptr;
	}
	if (BossWidget)
	{
		BossWidget->ShowFight();
	}
	bIntroPlaying = false;
	bReturning = false;
	SetGameHudHidden(TEXT("Intro"), false);
	if (bRestoreInput && GetData())
	{
		PlayBgm(GetData()->BossBgm);
	}
	SetIntroStage(ESoulBossIntroStage::Finished);
	SW_LOG("[PRES] boss intro done skipped=%d input=%d", bSkipped, PC && PC->InputEnabled());
}

void USoulPresentationSubsystem::AbortIntroForDeath()
{
	if (!bIntroPlaying)
		return;
	GetWorld()->GetTimerManager().ClearTimer(IntroTimer);
	GetWorld()->GetTimerManager().ClearTimer(ClipStopTimer);
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController(); PC && PC->GetPawn())
	{
		PC->SetViewTarget(PC->GetPawn());
	}
	SW_LOG("[PRES] boss intro aborted (player died)");
	FinishIntro(false);
}

void USoulPresentationSubsystem::ClearBoss()
{
	if (bIntroPlaying)
	{
		GetWorld()->GetTimerManager().ClearTimer(IntroTimer);
		FinishIntro(false);
	}
	if (BossWidget)
	{
		BossWidget->RemoveFromParent();
		BossWidget = nullptr;
	}
	IntroBoss.Reset();
}

void USoulPresentationSubsystem::HandleBossDied(AActor* Boss)
{
	if (BossWidget)
	{
		BossWidget->RemoveFromParent();
		BossWidget = nullptr;
	}
	if (bIntroPlaying)
	{
		GetWorld()->GetTimerManager().ClearTimer(IntroTimer);
		FinishIntro();
	}
}

void USoulPresentationSubsystem::Tick(float DeltaTime)
{
	// 스킵: DA의 키를 길게 누른다 (입력 잠금 중에도 키 상태는 갱신된다)
	const USoulPresentationData* Data = GetData();
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!Data || !PC || bReturning || Data->BossIntroSkipHoldSec <= 0.f)
		return;
	float Held = 0.f;
	for (const FKey& Key : Data->BossIntroSkipKeys)
	{
		Held = FMath::Max(Held, PC->GetInputKeyTimeDown(Key));
	}
	if (BossWidget)
	{
		BossWidget->SetSkipProgress(Held / Data->BossIntroSkipHoldSec);
	}
	if (Held >= Data->BossIntroSkipHoldSec)
	{
		SkipIntro();
	}
}

#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorld GSoulPresSkip(
	TEXT("soul.Pres.SkipIntro"), TEXT("보스 인트로를 바로 건너뛴다"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (USoulPresentationSubsystem* Pres = USoulPresentationSubsystem::Get(World))
		{
			Pres->SkipIntro();
		}
	}));
#endif
