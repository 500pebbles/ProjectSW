#include "Run/Progression/SoulProgressionComponent.h"
#include "Run/Progression/SoulLevelBarWidget.h"
#include "Run/Progression/SoulProgressionData.h"
#include "Run/Core/SoulRunConfig.h"
#include "Run/Core/SoulRunGameComponent.h"
#include "Run/Core/SoulRunSubsystem.h"
#include "Run/Mob/SoulMobComponent.h"
#include "Run/Mob/SoulMobEncounterSubsystem.h"
#include "Character/SoulCharacterEnemy.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Player/SoulPlayerState.h"
#include "Utility/SoulLog.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

USoulProgressionComponent::USoulProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

USoulProgressionComponent* USoulProgressionComponent::Find(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<USoulProgressionComponent>() : nullptr;
}

const USoulProgressionData* USoulProgressionComponent::GetData() const
{
	const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(this);
	return Config ? Config->Progression.Get() : nullptr;
}

UAbilitySystemComponent* USoulProgressionComponent::GetASC() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}

int32 USoulProgressionComponent::GetLevel() const
{
	const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
	return Run ? Run->GetState().Level : 1;
}

int32 USoulProgressionComponent::GetExp() const
{
	const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
	return Run ? Run->GetState().Exp : 0;
}

int32 USoulProgressionComponent::GetExpToNext() const
{
	const USoulProgressionData* Data = GetData();
	if (!Data || GetLevel() >= Data->MaxLevel)
		return 0;
	return FMath::RoundToInt(Data->EvalPlayerLevel(Data->ToNextRow, GetLevel()));
}

// ---------- 초기화 (플랜 2.3: 컨트롤러가 생긴 다음 틱) ----------

void USoulProgressionComponent::BeginPlay()
{
	Super::BeginPlay();

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !GetWorld()->IsGameWorld())
		return;

	if (Pawn->GetController())
	{
		ScheduleInitialize();
	}
	else
	{
		Pawn->ReceiveControllerChangedDelegate.AddUniqueDynamic(this, &ThisClass::HandleControllerChanged);
	}

	if (USoulMobEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<USoulMobEncounterSubsystem>())
	{
		MobKilledHandle = Encounter->OnMobKilled.AddUObject(this, &ThisClass::HandleMobKilled);
	}
}

void USoulProgressionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (USoulMobEncounterSubsystem* Encounter = World->GetSubsystem<USoulMobEncounterSubsystem>())
		{
			Encounter->OnMobKilled.Remove(MobKilledHandle);
		}
	}
	if (LevelBar)
	{
		LevelBar->RemoveFromParent();
		LevelBar = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void USoulProgressionComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	if (NewController)
	{
		ScheduleInitialize();
	}
}

void USoulProgressionComponent::ScheduleInitialize()
{
	// 빙의 콜백과 같은 틱에는 ASC 액터 정보가 아직 없을 수 있어 다음 틱에 건다
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::InitializeProgression);
}

void USoulProgressionComponent::InitializeProgression()
{
	if (bInitialized || !GetASC())
		return;
	bInitialized = true;

	ApplyLevel(GetLevel(), false);

	// 처음 들어올 때는 체력을 가득 채운다 (레벨 스탯이 최대 체력을 바꿨으므로)
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->SetNumericAttributeBase(USoulAttributeSet::GetHealthAttribute(), ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute()));
	}

	const USoulProgressionData* Data = GetData();
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (Data && Data->LevelBarClass && PC && PC->IsLocalController() && !LevelBar)
	{
		LevelBar = CreateWidget<USoulLevelBarWidget>(PC, Data->LevelBarClass);
		if (LevelBar)
		{
			LevelBar->AddToViewport(Data->LevelBarZOrder);
			LevelBar->Bind(this);
		}
	}
	Broadcast();
}

// ---------- 레벨 ----------

void USoulProgressionComponent::ApplyLevel(int32 Level, bool bLevelUp)
{
	const USoulProgressionData* Data = GetData();
	UAbilitySystemComponent* ASC = GetASC();
	if (!Data || !ASC)
		return;

	const float OldMax = ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute());
	if (Data->LevelStatsEffect)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(GetOwner());
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Data->LevelStatsEffect, Level, Context);
		if (Spec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	if (ASoulPlayerState* PS = Cast<APawn>(GetOwner()) ? Cast<APawn>(GetOwner())->GetPlayerState<ASoulPlayerState>() : nullptr)
	{
		PS->SetPlayerLevel(Level);
	}

	if (bLevelUp)
	{
		const float NewMax = ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute());
		const float Heal = FMath::Max(0.f, NewMax - OldMax) + NewMax * Data->LevelUpHealRatio;
		const float Health = ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute());
		ASC->SetNumericAttributeBase(USoulAttributeSet::GetHealthAttribute(), FMath::Min(Health + Heal, NewMax));
		SW_LOG("[LEVEL] up %d hp=%.0f/%.0f ap=%.0f def=%.0f", Level, ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute()), NewMax,
			ASC->GetNumericAttribute(USoulAttributeSet::GetAttackPowerAttribute()), ASC->GetNumericAttribute(USoulAttributeSet::GetDefenseAttribute()));
		OnLevelUp.Broadcast(Level);
	}
}

void USoulProgressionComponent::AddExp(int32 Amount, FName Reason)
{
	USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
	const USoulProgressionData* Data = GetData();
	if (!Run || !Data || Amount <= 0)
		return;

	int32 Level = Run->GetState().Level;
	int32 Exp = Run->GetState().Exp + Amount;
	while (Level < Data->MaxLevel)
	{
		const int32 ToNext = FMath::RoundToInt(Data->EvalPlayerLevel(Data->ToNextRow, Level));
		if (ToNext <= 0 || Exp < ToNext)
			break;
		Exp -= ToNext;
		++Level;
		Run->SetLevelExp(Level, Exp);
		ApplyLevel(Level, true);
	}
	if (Level >= Data->MaxLevel)
	{
		Exp = 0;
	}
	Run->SetLevelExp(Level, Exp);
	SW_LOG("[EXP] %s +%d -> lv=%d exp=%d/%d", *Reason.ToString(), Amount, Level, Exp, GetExpToNext());
	Broadcast();
}

void USoulProgressionComponent::SetLevel(int32 NewLevel)
{
	USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
	const USoulProgressionData* Data = GetData();
	if (!Run || !Data)
		return;
	NewLevel = FMath::Clamp(NewLevel, 1, Data->MaxLevel);
	Run->SetLevelExp(NewLevel, 0);
	ApplyLevel(NewLevel, true);
	Broadcast();
}

void USoulProgressionComponent::HandleMobKilled(ASoulCharacterEnemy* Enemy, const USoulMobComponent* Mob)
{
	const USoulProgressionData* Data = GetData();
	if (!Data || !Enemy || !Mob)
		return;
	const int32 MobLevel = Enemy->GetCharacterLevel();
	const int32 Amount = FMath::RoundToInt(Data->EvalKillBase(MobLevel) * Mob->GetExpWeight() * Data->ExpScale);
	SW_LOG("[EXP] mob=%s lv=%d w=%.0f +%d", *Enemy->GetClass()->GetName(), MobLevel, Mob->GetExpWeight(), Amount);
	AddExp(Amount, TEXT("Kill"));
}

void USoulProgressionComponent::Broadcast()
{
	OnProgressionChanged.Broadcast(GetLevel(), GetExp(), GetExpToNext());
}

void USoulProgressionComponent::DumpToLog() const
{
	const UAbilitySystemComponent* ASC = GetASC();
	SW_LOG("[LEVEL] dump lv=%d exp=%d/%d hp=%.0f/%.0f ap=%.0f def=%.0f stamina=%.0f/%.0f", GetLevel(), GetExp(), GetExpToNext(),
		ASC ? ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute()) : -1.f, ASC ? ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute()) : -1.f,
		ASC ? ASC->GetNumericAttribute(USoulAttributeSet::GetAttackPowerAttribute()) : -1.f, ASC ? ASC->GetNumericAttribute(USoulAttributeSet::GetDefenseAttribute()) : -1.f,
		ASC ? ASC->GetNumericAttribute(USoulAttributeSet::GetStaminaAttribute()) : -1.f, ASC ? ASC->GetNumericAttribute(USoulAttributeSet::GetMaxStaminaAttribute()) : -1.f);
}

#if !UE_BUILD_SHIPPING
static USoulProgressionComponent* FindPlayerProgression(UWorld* World)
{
	return USoulProgressionComponent::Find(UGameplayStatics::GetPlayerPawn(World, 0));
}

static FAutoConsoleCommandWithWorldAndArgs GSoulProgSetLevel(TEXT("soul.Prog.SetLevel"), TEXT("soul.Prog.SetLevel N : 런 레벨을 N으로"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (USoulProgressionComponent* P = FindPlayerProgression(World); P && Args.Num() > 0)
		{
			P->SetLevel(FCString::Atoi(*Args[0]));
			P->DumpToLog();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GSoulProgAddExp(TEXT("soul.Prog.AddExp"), TEXT("soul.Prog.AddExp N : 경험치를 N 더한다"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (USoulProgressionComponent* P = FindPlayerProgression(World); P && Args.Num() > 0)
		{
			P->AddExp(FCString::Atoi(*Args[0]), TEXT("Cheat"));
			P->DumpToLog();
		}
	}));

static FAutoConsoleCommandWithWorld GSoulProgDump(TEXT("soul.Prog.Dump"), TEXT("런 레벨과 스탯을 로그로"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (const USoulProgressionComponent* P = FindPlayerProgression(World))
		{
			P->DumpToLog();
		}
	}));
#endif
