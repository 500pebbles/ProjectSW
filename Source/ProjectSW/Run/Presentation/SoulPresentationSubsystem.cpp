#include "Run/Presentation/SoulPresentationSubsystem.h"
#include "Run/Presentation/SoulPresentationData.h"
#include "Run/Presentation/SoulTransitionSubsystem.h"
#include "Run/Core/SoulRunConfig.h"
#include "Run/Core/SoulRunGameComponent.h"
#include "Run/UI/SoulPresentationWidgets.h"
#include "Character/SoulCharacterEnemy.h"
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

void USoulPresentationSubsystem::PlayBossIntro(ASoulCharacterEnemy* Boss)
{
	const USoulPresentationData* Data = GetData();
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!Data || !Boss || !PC || bIntroPlaying)
		return;

	IntroBoss = Boss;
	Boss->OnDied.AddUObject(this, &ThisClass::HandleBossDied);
	const FSoulBossIntro* Intro = Data->FindBossIntro(Boss);

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
	bReturning = false;
	bSkipped = false;
	USoulTransitionSubsystem::PrepareForTravel(PC);
	PC->DisableInput(PC);
	if (APawn* Pawn = PC->GetPawn())
	{
		Pawn->DisableInput(PC);
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
	PlaySound2D(Data->BossDoorSound);
	PlaySound2D(Data->WarningSound);
	SetIntroStage(ESoulBossIntroStage::Start);
	GetWorld()->GetTimerManager().SetTimer(IntroTimer, this, &ThisClass::IntroCameraStage, FMath::Max(Data->BossIntroLeadSec, 0.01f), false);
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
		BossWidget->ShowNameCard(Intro->Name, Intro->Title);
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
			Anim->PlaySlotAnimationAsDynamicMontage(Intro->Clip, TEXT("DefaultSlot"), 0.15f, 0.2f, 1.f, 1, -1.f, Intro->ClipStart);
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
	PlayAt(Intro->Sound, Intro->FX, Boss->GetActorLocation());
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
	GetWorld()->GetTimerManager().SetTimer(IntroTimer, this, &ThisClass::FinishIntro, FMath::Max(Blend, 0.01f), false);
}

void USoulPresentationSubsystem::SkipIntro()
{
	if (!bIntroPlaying || bReturning || bSkipped)
		return;
	bSkipped = true;
	GetWorld()->GetTimerManager().ClearTimer(IntroTimer);
	GetWorld()->GetTimerManager().ClearTimer(ClipStopTimer);
	const USoulPresentationData* Data = GetData();
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

void USoulPresentationSubsystem::FinishIntro()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		PC->EnableInput(PC);
		if (APawn* Pawn = PC->GetPawn())
		{
			Pawn->EnableInput(PC);
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
	SetIntroStage(ESoulBossIntroStage::Finished);
	SW_LOG("[PRES] boss intro done skipped=%d input=%d", bSkipped, PC && PC->InputEnabled());
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
