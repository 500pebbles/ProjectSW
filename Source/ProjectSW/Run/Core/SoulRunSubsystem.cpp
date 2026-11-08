#include "Run/Core/SoulRunSubsystem.h"
#include "Run/Core/SoulRunConfig.h"
#include "Run/Core/SoulRunGameComponent.h"
#include "Run/Equipment/SoulEquipmentComponent.h"
#include "Run/Equipment/SoulEquipmentDefinition.h"
#include "Run/Save/SoulHallOfFameSubsystem.h"
#include "Run/Save/SoulScoreRules.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Utility/SoulLog.h"
#include "Containers/Ticker.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

USoulRunSubsystem* USoulRunSubsystem::Get(const UObject* WorldContextObject)
{
	const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	return GameInstance ? GameInstance->GetSubsystem<USoulRunSubsystem>() : nullptr;
}

void USoulRunSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
#if !UE_BUILD_SHIPPING
	const TCHAR* Cmd = FCommandLine::Get();
	if (FParse::Param(Cmd, TEXT("SoulTownSelfTest")) || FParse::Param(Cmd, TEXT("SoulFloorSelfTest")))
	{
		// 층 워치독은 마을로 넘어가며 사라지므로, 게임 인스턴스와 함께 사는 실시간 워치독을 따로 둔다
		static bool bArmed = false;
		if (!bArmed)
		{
			bArmed = true;
			float TimeoutSec = 300.f;
			FParse::Value(Cmd, TEXT("SoulSelfTestTimeout="), TimeoutSec);
			const double Deadline = FPlatformTime::Seconds() + TimeoutSec;
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Deadline, TimeoutSec](float)
			{
				if (FPlatformTime::Seconds() < Deadline)
					return true;
				SW_LOG_ERROR("[SELFTEST] FAIL watchdog total %.0fs", TimeoutSec);
				GLog->Flush();
				FPlatformMisc::RequestExitWithStatus(true, 3);
				return false;
			}), 1.f);
			SW_LOG("[SELFTEST] total watchdog %.0fs", TimeoutSec);
		}
	}
#endif
}

void USoulRunSubsystem::BeginRun()
{
	State = FSoulRunState();
	State.Floor = 1;
	State.Level = 1;
	State.Exp = 0;
	// 층 시드에 층 번호와 재시도 횟수를 더하므로 int32 끝에서 넘치지 않게 범위를 둔다
	State.RunSeed = FMath::RandRange(1, 1000000000);
	State.StartRealTime = FPlatformTime::Seconds();
	State.bActive = true;
	State.bRunEnded = false;
	ForcedScore = -1;
	bSkipSubmit = false;

	SW_LOG("[RUN] BeginRun seed=%d", State.RunSeed);
}

void USoulRunSubsystem::EndRun(bool bSuccess)
{
	if (!State.bActive || State.bRunEnded)
	{
		SW_LOG("[RUN] EndRun(%d) 무시: active=%d ended=%d", bSuccess, State.bActive, State.bRunEnded);
		return;
	}

	State.bRunEnded = true;
	State.EndRealTime = FPlatformTime::Seconds();
	const double Elapsed = GetElapsedRealSeconds();
	SW_LOG("[RUN] EndRun success=%d floor=%d level=%d sec=%.1f", bSuccess, State.Floor, State.Level, Elapsed);
	if (State.FloorStartRealTime > 0.0)
	{
		State.Stats.FloorSeconds.Add(static_cast<float>(State.EndRealTime - State.FloorStartRealTime));
	}

	// 정산: 원장 + 클리어 정산(레벨, 시간) + 랭크 (DA_ScoreRules)
	LastSummary = FSoulRunSummary();
	LastSummary.bSuccess = bSuccess;
	LastSummary.ClearSeconds = static_cast<float>(Elapsed);
	LastSummary.FinalLevel = State.Level;
	LastSummary.FloorReached = State.Floor;
	LastSummary.Stats = State.Stats;
	LastSummary.FloorSeeds = State.FloorSeeds;
	LastSummary.CostumeId = SelectedCostume;
	const APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (const USoulEquipmentComponent* Equipment = USoulEquipmentComponent::Find(PC ? PC->GetPawn() : nullptr))
	{
		if (const USoulEquipmentDefinition* Weapon = Equipment->GetCurrentWeapon())
		{
			LastSummary.WeaponId = Weapon->GetFName();
			LastSummary.WeaponElementTag = Weapon->ElementTag;
		}
		if (!Equipment->GetCurrentCostumeId().IsNone())
		{
			LastSummary.CostumeId = Equipment->GetCurrentCostumeId();
		}
	}
	const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr);
	if (Config && Config->ScoreRules)
	{
		Config->ScoreRules->Calculate(State.Ledger, LastSummary);
	}
	else
	{
		static bool bWarned = false;
		if (!bWarned)
		{
			bWarned = true;
			SW_LOG_WARNING("[RUN] DA_RunConfig.ScoreRules가 비어 있음: 원장 합만 점수로 쓰고 랭크 없음");
		}
		LastSummary.Score = GetLedgerTotal();
		for (const TPair<FName, int32>& Pair : State.Ledger)
		{
			LastSummary.Breakdown.Add({ Pair.Key, Pair.Value });
		}
	}
	if (ForcedScore >= 0)
	{
		LastSummary.Score = ForcedScore;
		LastSummary.RankId = bSuccess && Config && Config->ScoreRules ? Config->ScoreRules->FindRank(ForcedScore) : FName(NAME_None);
	}
	for (const FSoulScoreLine& Line : LastSummary.Breakdown)
	{
		SW_LOG("[RUN] Summary %s=%d", *Line.Category.ToString(), Line.Score);
	}
	SW_LOG("[RUN] Summary ledger=%d forced=%d score=%d rank=%s costume=%s weapon=%s kills=%d events=%d damage=%.0f", GetLedgerTotal(), ForcedScore, LastSummary.Score,
		*LastSummary.RankId.ToString(), *LastSummary.CostumeId.ToString(), *LastSummary.WeaponId.ToString(), LastSummary.Stats.Kills, LastSummary.Stats.Events, LastSummary.Stats.DamageTaken);

	// 치트로 덮은 점수는 실제 명예의 전당에 넣지 않는다
	if (USoulHallOfFameSubsystem* HoF = GetGameInstance() ? GetGameInstance()->GetSubsystem<USoulHallOfFameSubsystem>() : nullptr; HoF && ForcedScore < 0 && !bSkipSubmit)
	{
		HoF->SubmitRun(LastSummary);
	}

	OnRunEnded.Broadcast(bSuccess);

	State.Level = 1;
	State.Exp = 0;
}

void USoulRunSubsystem::SetFloor(int32 NewFloor)
{
	State.Floor = NewFloor;
}

void USoulRunSubsystem::NotifyFloorGenerated(int32 Seed)
{
	const double Now = FPlatformTime::Seconds();
	if (State.FloorStartRealTime > 0.0)
	{
		State.Stats.FloorSeconds.Add(static_cast<float>(Now - State.FloorStartRealTime));
	}
	State.FloorStartRealTime = Now;
	State.FloorSeeds.Add(Seed);
}

void USoulRunSubsystem::NotifyItemPickup(FName ItemRow, bool bWeapon)
{
	if (!IsRunActive())
		return;
	const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr);
	const USoulScoreRules* Rules = Config ? Config->ScoreRules.Get() : nullptr;
	if (!Rules)
		return;
	if (bWeapon)
	{
		if (!State.ScoredWeapons.Contains(ItemRow) && State.ScoredWeapons.Num() < Rules->MaxScoredWeapons)
		{
			State.ScoredWeapons.Add(ItemRow);
			AddScore(TEXT("Weapon"), Rules->WeaponFirstPickupScore);
		}
		return;
	}
	if (const int32* Score = Rules->PickupScores.Find(ItemRow))
	{
		AddScore(TEXT("Pickup"), *Score);
	}
}

void USoulRunSubsystem::SetLevelExp(int32 NewLevel, int32 NewExp)
{
	State.Level = NewLevel;
	State.Exp = NewExp;
}

void USoulRunSubsystem::AddScore(FName Category, int32 Amount)
{
	if (!IsRunActive() || Amount == 0)
		return;
	State.Ledger.FindOrAdd(Category) += Amount;
	SW_LOG("[SCORE] %s +%d total=%d", *Category.ToString(), Amount, GetLedgerTotal());
}

int32 USoulRunSubsystem::GetLedgerTotal() const
{
	int32 Total = 0;
	for (const TPair<FName, int32>& Pair : State.Ledger)
	{
		Total += Pair.Value;
	}
	return Total;
}

double USoulRunSubsystem::GetElapsedRealSeconds() const
{
	if (!State.bActive)
		return 0.0;
	return (State.bRunEnded ? State.EndRealTime : FPlatformTime::Seconds()) - State.StartRealTime;
}
