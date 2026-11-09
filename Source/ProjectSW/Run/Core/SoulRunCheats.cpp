// 런 치트 (개발 빌드 전용)
#include "Run/Core/SoulRunSubsystem.h"
#include "Run/Flow/SoulFloorDirectorComponent.h"
#include "Run/Save/SoulHallOfFameSubsystem.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Interface/SoulCombatInterface.h"
#include "Utility/SoulLog.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

#if !UE_BUILD_SHIPPING
namespace SoulRunCheats
{
	/** 폰에 즉시 GE로 큰 IncomingDamage를 넣어 팀 사망 경로(AttributeSet → Die → OnDied)를 그대로 탄다 */
	static void KillPlayer(UWorld* World)
	{
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
		if (!ASC)
		{
			SW_LOG_WARNING("soul.Run.ForceEnd Fail : 폰 ASC 없음");
			return;
		}
		if (Pawn->Implements<USoulCombatInterface>() && ISoulCombatInterface::Execute_IsDead(Pawn))
		{
			SW_LOG_WARNING("soul.Run.ForceEnd Fail : 이미 죽은 폰");
			return;
		}

		UGameplayEffect* Kill = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_SoulForceKill"));
		Kill->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Modifier = Kill->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = USoulAttributeSet::GetIncomingDamageAttribute();
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(1000000.f);

		ASC->ApplyGameplayEffectToSelf(Kill, 1.f, ASC->MakeEffectContext());
		SW_LOG("[RUN] ForceEnd Fail: 폰 Health %.0f", ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute()));
	}
}

static FAutoConsoleCommandWithWorldAndArgs GSoulRunForceEndCommand(
	TEXT("soul.Run.ForceEnd"),
	TEXT("soul.Run.ForceEnd Clear [Score] : EndRun(성공)을 바로 부른다. soul.Run.ForceEnd Fail : 폰 체력을 0으로 만들어 사망 경로를 탄다"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const FString Mode = Args.Num() > 0 ? Args[0] : TEXT("Clear");
		if (Mode.Equals(TEXT("Fail"), ESearchCase::IgnoreCase))
		{
			SoulRunCheats::KillPlayer(World);
			return;
		}

		USoulRunSubsystem* Run = USoulRunSubsystem::Get(World);
		if (!Run)
			return;
		if (Args.Num() > 1)
		{
			Run->SetForcedScore(FCString::Atoi(*Args[1]));
		}
		// 점수 인자와 상관없이 기록을 내지 않고, 결과창이 직전 실제 기록을 고치지 않게 한다
		Run->SetSkipSubmit(true);
		if (USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(World))
		{
			HoF->ClearLastSubmitted();
		}
		SW_LOG("[RUN] ForceEnd Clear score=%s", Args.Num() > 1 ? *Args[1] : TEXT("(계산)"));
		// 던전이면 정상 클리어 경로(결과창), 아니면 바로 EndRun
		if (USoulFloorDirectorComponent* Director = USoulFloorDirectorComponent::Get(World))
		{
			Director->DebugForceClear();
		}
		else
		{
			Run->EndRun(true);
		}
	}));
#endif
