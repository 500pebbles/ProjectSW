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

USoulRunSubsystem* USoulRunSubsystem::Get(const UObject* WorldContextObject)
{
	const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	return GameInstance ? GameInstance->GetSubsystem<USoulRunSubsystem>() : nullptr;
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

	if (USoulHallOfFameSubsystem* HoF = GetGameInstance() ? GetGameInstance()->GetSubsystem<USoulHallOfFameSubsystem>() : nullptr)
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
