#include "Run/Save/SoulHallOfFameSubsystem.h"
#include "Run/Save/SoulHallOfFameSave.h"
#include "Run/Save/SoulHallOfFameRules.h"
#include "Run/Core/SoulRunConfig.h"
#include "Run/Core/SoulRunGameComponent.h"
#include "Run/Core/SoulRunSubsystem.h"
#include "Utility/SoulLog.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"

const TCHAR* USoulHallOfFameSubsystem::SlotName = TEXT("SoulProfile");
const TCHAR* USoulHallOfFameSubsystem::BackupSlotName = TEXT("SoulProfile_Bak");

USoulHallOfFameSubsystem* USoulHallOfFameSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<USoulHallOfFameSubsystem>() : nullptr;
}

void USoulHallOfFameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<USoulRunSubsystem>();
	Super::Initialize(Collection);
	Load();

	// 재시작 때 마지막 의상을 런 상태에 넣어 마을 입장에서 그대로 입힌다
	if (USoulRunSubsystem* Run = GetGameInstance()->GetSubsystem<USoulRunSubsystem>(); Run && Profile && !Profile->LastCostumeId.IsNone())
	{
		Run->SetSelectedCostume(Profile->LastCostumeId);
	}
}

const USoulHallOfFameRules* USoulHallOfFameSubsystem::GetRules() const
{
	if (!CachedRules)
	{
		const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr);
		CachedRules = Config ? Config->HallOfFameRules.Get() : nullptr;
		static bool bWarned = false;
		if (Config && !CachedRules && !bWarned)
		{
			bWarned = true;
			SW_LOG_WARNING("[HOF] DA_RunConfig.HallOfFameRules가 비어 있음 (최대 행, 이름 길이, 기본 이름 없음)");
		}
	}
	return CachedRules;
}

void USoulHallOfFameSubsystem::Load()
{
	LoadNotice.Reset();
	const bool bMainExists = UGameplayStatics::DoesSaveGameExist(SlotName, 0);
	auto LoadChecked = [](const TCHAR* Slot) -> USoulHallOfFameSave*
	{
		USoulHallOfFameSave* Loaded = Cast<USoulHallOfFameSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
		if (!Loaded)
			return nullptr;
		// 표식 이전 버전(1)은 표식 없이 받는다. 2부터는 표식이 맞아야 한다
		if (Loaded->SaveVersion < USoulHallOfFameSave::FirstMarkerVersion)
			return Loaded;
		return Loaded->EndMarker == USoulHallOfFameSave::EndMarkerValue ? Loaded : nullptr;
	};
	Profile = bMainExists ? LoadChecked(SlotName) : nullptr;
	bMainValid = Profile != nullptr;
	if (!Profile && bMainExists)
	{
		// 깨진 본 파일은 .corrupt.sav로 남기고 백업을 읽는다
		const FString Dir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
		IFileManager::Get().Copy(*(Dir / TEXT("SoulProfile.corrupt.sav")), *(Dir / TEXT("SoulProfile.sav")));
		Profile = LoadChecked(BackupSlotName);
		LoadNotice = Profile ? TEXT("세이브가 손상되어 백업에서 복구했습니다") : TEXT("세이브가 손상되어 새 프로필로 시작합니다");
		SW_LOG_WARNING("[HOF] 본 세이브 손상, 백업 %s", Profile ? TEXT("복구") : TEXT("없음"));
	}
	if (!Profile)
	{
		Profile = Cast<USoulHallOfFameSave>(UGameplayStatics::CreateSaveGameObject(USoulHallOfFameSave::StaticClass()));
		SW_LOG("[HOF] 새 프로필");
	}
	SW_LOG("[HOF] 로드 entries=%d runs=%d clears=%d name=%s costume=%s", Profile->HallOfFame.Num(), Profile->TotalRuns, Profile->TotalClears,
		*Profile->LastPlayerName, *Profile->LastCostumeId.ToString());
}

void USoulHallOfFameSubsystem::Save()
{
	if (!Profile)
		return;
	Profile->SaveVersion = USoulHallOfFameSave::CurrentVersion;
	Profile->EndMarker = USoulHallOfFameSave::EndMarkerValue;
	// 엔진 쓰기가 원자적이 아니라, 직전 정상본을 백업으로 돌린 뒤 본을 새로 쓴다
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	const FString MainFile = Dir / FString(SlotName) + TEXT(".sav");
	bool bBackup = false;
	// 본이 정상일 때만 백업으로 돌린다 (백업으로 복구한 직후 깨진 본이 정상 백업을 덮지 않게)
	if (bMainValid && IFileManager::Get().FileExists(*MainFile))
	{
		bBackup = IFileManager::Get().Copy(*(Dir / FString(BackupSlotName) + TEXT(".sav")), *MainFile) == COPY_OK;
	}
	const bool bMain = UGameplayStatics::SaveGameToSlot(Profile, SlotName, 0);
	bMainValid = bMain;
	if (!bBackup && bMain)
	{
		bBackup = UGameplayStatics::SaveGameToSlot(Profile, BackupSlotName, 0);
	}
	SW_LOG("[HOF] 저장 main=%d backup=%d entries=%d", bMain, bBackup, Profile->HallOfFame.Num());
	OnChanged.Broadcast();
}

void USoulHallOfFameSubsystem::SortAndTrim()
{
	Profile->HallOfFame.Sort([](const FSoulHallOfFameEntry& A, const FSoulHallOfFameEntry& B)
	{
		if (A.Score != B.Score)
			return A.Score > B.Score;
		if (A.ClearSeconds != B.ClearSeconds)
			return A.ClearSeconds < B.ClearSeconds;
		return A.DateUtc < B.DateUtc;
	});
	if (const USoulHallOfFameRules* Rules = GetRules(); Rules && Profile->HallOfFame.Num() > Rules->MaxEntries)
	{
		Profile->HallOfFame.SetNum(Rules->MaxEntries);
	}
}

FString USoulHallOfFameSubsystem::CleanName(const FString& Name) const
{
	FString Clean = Name.TrimStartAndEnd();
	if (const USoulHallOfFameRules* Rules = GetRules())
	{
		Clean = Clean.Left(Rules->NameMaxLength);
		if (Clean.IsEmpty())
		{
			Clean = Rules->DefaultName;
		}
	}
	return Clean;
}

FGuid USoulHallOfFameSubsystem::SubmitRun(const FSoulRunSummary& Summary)
{
	LastSubmittedId.Invalidate();
	bLastSubmittedBest = false;
	if (!Profile)
		return LastSubmittedId;

	++Profile->TotalRuns;
	Profile->TotalClears += Summary.bSuccess ? 1 : 0;
	Profile->BestFloor = FMath::Max(Profile->BestFloor, Summary.FloorReached);

	const USoulHallOfFameRules* Rules = GetRules();
	if (Summary.bSuccess || (Rules && Rules->bIncludeFailures))
	{
		const int32 PreviousBest = Profile->HallOfFame.IsEmpty() ? TNumericLimits<int32>::Lowest() : Profile->HallOfFame[0].Score;
		FSoulHallOfFameEntry& Entry = Profile->HallOfFame.AddDefaulted_GetRef();
		Entry.EntryId = FGuid::NewGuid();
		Entry.PlayerName = CleanName(Profile->LastPlayerName);
		Entry.Score = Summary.Score;
		Entry.RankId = Summary.RankId;
		Entry.bSuccess = Summary.bSuccess;
		Entry.ClearSeconds = Summary.ClearSeconds;
		Entry.FinalLevel = Summary.FinalLevel;
		Entry.CostumeId = Summary.CostumeId;
		Entry.WeaponId = Summary.WeaponId;
		Entry.WeaponElementTag = Summary.WeaponElementTag;
		Entry.Stats = Summary.Stats;
		Entry.FloorSeeds = Summary.FloorSeeds;
		Entry.ScoreBreakdown = Summary.Breakdown;
		Entry.DateUtc = FDateTime::UtcNow();
		LastSubmittedId = Entry.EntryId;
		bLastSubmittedBest = Summary.Score > PreviousBest;
		SortAndTrim();
		if (GetRankOf(LastSubmittedId) == 0)
		{
			LastSubmittedId.Invalidate();
		}
	}
	SW_LOG("[HOF] submit success=%d score=%d rank=%s place=%d best=%d", Summary.bSuccess, Summary.Score, *Summary.RankId.ToString(), GetRankOf(LastSubmittedId), bLastSubmittedBest);
	// 이름 입력 중 꺼져도 기록이 남도록 먼저 저장한다
	Save();
	return LastSubmittedId;
}

void USoulHallOfFameSubsystem::SetEntryName(FGuid EntryId, const FString& Name)
{
	if (!Profile)
		return;
	const FString Clean = CleanName(Name);
	Profile->LastPlayerName = Clean;
	for (FSoulHallOfFameEntry& Entry : Profile->HallOfFame)
	{
		if (Entry.EntryId == EntryId)
		{
			Entry.PlayerName = Clean;
		}
	}
	SW_LOG("[HOF] name %s", *Clean);
	Save();
}

void USoulHallOfFameSubsystem::SetLastCostume(FName CostumeId)
{
	if (!Profile || Profile->LastCostumeId == CostumeId)
		return;
	Profile->LastCostumeId = CostumeId;
	Save();
}

void USoulHallOfFameSubsystem::ResetRecords()
{
	if (!Profile)
		return;
	Profile->HallOfFame.Reset();
	LastSubmittedId.Invalidate();
	SW_LOG("[HOF] 기록 초기화");
	Save();
	// 백업에 옛 기록이 남지 않게 백업도 빈 기록으로 쓴다
	const bool bBackup = UGameplayStatics::SaveGameToSlot(Profile, BackupSlotName, 0);
	SW_LOG("[HOF] 초기화 백업도 씀=%d", bBackup);
}

TArray<FSoulHallOfFameEntry> USoulHallOfFameSubsystem::GetTopEntries(int32 Count) const
{
	TArray<FSoulHallOfFameEntry> Out;
	if (Profile)
	{
		for (int32 i = 0; i < FMath::Min(Count, Profile->HallOfFame.Num()); ++i)
		{
			Out.Add(Profile->HallOfFame[i]);
		}
	}
	return Out;
}

int32 USoulHallOfFameSubsystem::GetRankOf(FGuid EntryId) const
{
	if (!Profile || !EntryId.IsValid())
		return 0;
	return Profile->HallOfFame.IndexOfByPredicate([EntryId](const FSoulHallOfFameEntry& Entry) { return Entry.EntryId == EntryId; }) + 1;
}

int32 USoulHallOfFameSubsystem::GetEntryCount() const
{
	return Profile ? Profile->HallOfFame.Num() : 0;
}

FString USoulHallOfFameSubsystem::GetLastPlayerName() const
{
	if (Profile && !Profile->LastPlayerName.IsEmpty())
		return Profile->LastPlayerName;
	const USoulHallOfFameRules* Rules = GetRules();
	return Rules ? Rules->DefaultName : FString();
}

FName USoulHallOfFameSubsystem::GetLastCostumeId() const
{
	return Profile ? Profile->LastCostumeId : NAME_None;
}

int32 USoulHallOfFameSubsystem::GetBoardRows() const
{
	const USoulHallOfFameRules* Rules = GetRules();
	return Rules ? Rules->BoardRows : 0;
}

int32 USoulHallOfFameSubsystem::GetNameMaxLength() const
{
	const USoulHallOfFameRules* Rules = GetRules();
	return Rules ? Rules->NameMaxLength : 0;
}

void USoulHallOfFameSubsystem::AddTestEntries(int32 Count)
{
	if (!Profile)
		return;
	static const TCHAR* Names[] = { TEXT("치이"), TEXT("Haru"), TEXT("에르나"), TEXT("Lily"), TEXT("스텔라"), TEXT("Jin") };
	static const TCHAR* Costumes[] = { TEXT("DA_Costume_Default"), TEXT("DA_Costume_DesireWalker"), TEXT("DA_Costume_Fenrir"), TEXT("DA_Costume_ShadowWhisper") };
	static const TCHAR* Weapons[] = { TEXT("DA_Weapon_012"), TEXT("DA_Weapon_029"), TEXT("DA_Weapon_015") };
	for (int32 i = 0; i < Count; ++i)
	{
		FSoulHallOfFameEntry& Entry = Profile->HallOfFame.AddDefaulted_GetRef();
		Entry.EntryId = FGuid::NewGuid();
		Entry.PlayerName = Names[FMath::RandRange(0, UE_ARRAY_COUNT(Names) - 1)];
		Entry.Score = FMath::RandRange(230000, 275000);
		Entry.ClearSeconds = FMath::FRandRange(900.f, 1500.f);
		Entry.FinalLevel = FMath::RandRange(12, 15);
		Entry.CostumeId = Costumes[FMath::RandRange(0, UE_ARRAY_COUNT(Costumes) - 1)];
		Entry.WeaponId = Weapons[FMath::RandRange(0, UE_ARRAY_COUNT(Weapons) - 1)];
		Entry.DateUtc = FDateTime::UtcNow();
		Entry.RankId = Entry.Score >= 263000 ? FName(TEXT("S")) : Entry.Score >= 253000 ? FName(TEXT("A")) : Entry.Score >= 233000 ? FName(TEXT("B")) : FName(TEXT("C"));
	}
	SortAndTrim();
	Save();
}

void USoulHallOfFameSubsystem::DumpToLog() const
{
	if (!Profile)
		return;
	SW_LOG("[HOF] dump entries=%d runs=%d clears=%d bestFloor=%d last=%s costume=%s", Profile->HallOfFame.Num(), Profile->TotalRuns, Profile->TotalClears, Profile->BestFloor,
		*Profile->LastPlayerName, *Profile->LastCostumeId.ToString());
	for (int32 i = 0; i < Profile->HallOfFame.Num(); ++i)
	{
		const FSoulHallOfFameEntry& E = Profile->HallOfFame[i];
		SW_LOG("[HOF] %d %s score=%d rank=%s sec=%.0f lv=%d costume=%s weapon=%s", i + 1, *E.PlayerName, E.Score, *E.RankId.ToString(), E.ClearSeconds, E.FinalLevel,
			*E.CostumeId.ToString(), *E.WeaponId.ToString());
	}
}

#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorldAndArgs GSoulHoFAddTest(
	TEXT("soul.HoF.AddTest"), TEXT("soul.HoF.AddTest <개수> : 시험 기록을 넣고 저장한다"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(World))
		{
			HoF->AddTestEntries(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 3);
			HoF->DumpToLog();
		}
	}));

static FAutoConsoleCommandWithWorld GSoulHoFReset(
	TEXT("soul.HoF.Reset"), TEXT("명예의 전당 기록을 모두 지우고 저장한다"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(World))
		{
			HoF->ResetRecords();
		}
	}));

static FAutoConsoleCommandWithWorld GSoulHoFDump(
	TEXT("soul.HoF.Dump"), TEXT("명예의 전당과 프로필을 로그로"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (const USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(World))
		{
			HoF->DumpToLog();
		}
	}));
#endif
