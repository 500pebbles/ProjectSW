#include "Build/PMGMapBuilder.h"
#include "Core/PMGAssets.h"
#include "Core/PMGLayout.h"
#include "Data/PMGSettings.h"
#include "PMGGenerator.h"
#include "PMGLog.h"
#include "Components/LineBatchComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"

namespace
{
	/**
	 * 결과의 방에 배정된 에셋 항목 (Phase 9, 10 결과 형식). 사용된 방은 방 타입의 크기별 리스트, 미사용 방은 채움 방 리스트의 항목.
	 * 스폰하지 않는 방(VariantIndex가 INDEX_NONE)이면 nullptr
	 */
	const FPMGAssetVariant* FindRoomVariantToSpawn(const FPMGAssetLists& AssetLists, const FPMGResultRoom& Room)
	{
		if (Room.VariantIndex == INDEX_NONE)
		{
			return nullptr;
		}
		if (!Room.bConnected)
		{
			return &AssetLists.FillerRooms.Variants[Room.VariantIndex];
		}
		const FPMGRoomTypeAssets* TypeAssets = AssetLists.FindRoomTypeAssets(Room.RoomType);   // 같은 설정으로 만든 결과라 항상 있다
		check(TypeAssets);
		return &TypeAssets->GetRoomAssets(PMG::GetRoomSize(Room.Shape)).Variants[Room.VariantIndex];
	}

	/** 결과의 커넥터에 배정된 에셋 항목. 문은 DoorListRoomType 타입의 Doors (None이면 전역 Doors), 벽은 Walls. 에셋이 없으면 nullptr */
	const FPMGAssetVariant* FindConnectorVariantToSpawn(const FPMGAssetLists& AssetLists, const FPMGResultConnector& Connector)
	{
		if (Connector.VariantIndex == INDEX_NONE)
		{
			return nullptr;
		}
		if (Connector.Type == EPMGConnectorType::Wall)
		{
			return &AssetLists.Walls.Variants[Connector.VariantIndex];
		}
		if (Connector.DoorListRoomType.IsNone())
		{
			return &AssetLists.Doors.Variants[Connector.VariantIndex];
		}
		const FPMGRoomTypeAssets* TypeAssets = AssetLists.FindRoomTypeAssets(Connector.DoorListRoomType);
		check(TypeAssets);
		return &TypeAssets->Doors.Variants[Connector.VariantIndex];
	}

	/**
	 * 미리보기에서 커스텀 타입 방을 표시하는 색. Settings의 CustomTypes 순서대로 돌려 쓴다 (Name은 로그에 남기는 색 이름).
	 * 구별이 쉬운 원색(RGB, CMY)만 쓴다. 흰색은 방 테두리와 입구/출구 기호가 쓰므로 뺀다
	 */
	struct FPreviewTypeColor
	{
		FColor Color;
		const TCHAR* Name;
	};
	const FPreviewTypeColor CustomTypePreviewColors[] =
	{
		{ FColor(255, 0, 0), TEXT("빨강") },
		{ FColor(0, 255, 0), TEXT("초록") },
		{ FColor(0, 0, 255), TEXT("파랑") },
		{ FColor(0, 255, 255), TEXT("시안") },
		{ FColor(255, 0, 255), TEXT("자홍") },
		{ FColor(255, 255, 0), TEXT("노랑") },
	};

	/** 커스텀 타입 TypeName의 미리보기 색. Settings의 CustomTypes에 없는 이름이면 nullptr */
	const FPreviewTypeColor* FindCustomTypePreviewColor(const UPMGSettings& Settings, FName TypeName)
	{
		const int32 TypeIndex = Settings.CustomTypes.IndexOfByPredicate([TypeName](const FPMGCustomRoomType& CustomType) { return CustomType.TypeName == TypeName; });
		if (TypeIndex == INDEX_NONE)
		{
			return nullptr;
		}
		return &CustomTypePreviewColors[TypeIndex % UE_ARRAY_COUNT(CustomTypePreviewColors)];
	}

	/** 결과에서 두 방 사이의 문 (어느 쪽이 RoomA여도 됨). 없으면 nullptr */
	const FPMGResultDoor* FindResultDoor(const FPMGResult& Result, int32 RoomA, int32 RoomB)
	{
		return Result.Doors.FindByPredicate([RoomA, RoomB](const FPMGResultDoor& Door)
		{
			return (Door.RoomA == RoomA && Door.RoomB == RoomB) || (Door.RoomA == RoomB && Door.RoomB == RoomA);
		});
	}
}

FTransform PMG::GetRoomSpawnTransform(const FPMGResultRoom& Room, float CellSize, const FVector& Origin)
{
	const FVector2D Center = GetRectCenter(Room.Bounds);   // 셀 단위. 셀 X = 월드 X, 셀 Y = 월드 Y
	const FVector Location = Origin + FVector(Center.X * CellSize, Center.Y * CellSize, 0.0);

	double Yaw = Room.Rotation * 90.0;
	if (Room.Shape == EPMGRoomShape::DualVertical)
	{
		Yaw += 90.0;   // 가로로 만든 2칸 에셋을 세로 방에 맞게 세운다
	}
	return FTransform(FRotator(0.0, Yaw, 0.0), Location);
}

FTransform PMG::GetConnectorSpawnTransform(const FPMGResultConnector& Connector, float CellSize, const FVector& Origin)
{
	// 셀 (X, Y)의 중심은 (X + 0.5, Y + 0.5). 맞닿은 두 셀 중심의 가운데가 경계의 중점이다
	const FVector2D EdgeCenter((Connector.CellA.X + Connector.CellB.X) * 0.5 + 0.5, (Connector.CellA.Y + Connector.CellB.Y) * 0.5 + 0.5);
	const FVector Location = Origin + FVector(EdgeCenter.X * CellSize, EdgeCenter.Y * CellSize, 0.0);

	// Yaw는 +X에서 +Y 쪽으로 도는 것이 양수 (언리얼 기준)
	const FIntPoint Direction = Connector.CellB - Connector.CellA;
	double Yaw = 0.0;   // +X
	if (Direction.X < 0)
	{
		Yaw = 180.0;
	}
	else if (Direction.Y > 0)
	{
		Yaw = 90.0;
	}
	else if (Direction.Y < 0)
	{
		Yaw = 270.0;
	}
	return FTransform(FRotator(0.0, Yaw, 0.0), Location);
}

TArray<FTransform> PMG::GetRoomOpeningTransforms(EPMGRoomShape Shape, float CellSize, const FVector& RoomCenter)
{
	// 방을 셀 (0, 0)부터 놓았다고 보고, 방 중심이 RoomCenter에 오도록 맵의 (0, 0) 셀 모서리(Origin)를 정한다
	const FIntRect Bounds(FIntPoint(0, 0), GetShapeCellCount(Shape));
	const FVector2D Center = GetRectCenter(Bounds);
	const FVector Origin = RoomCenter - FVector(Center.X * CellSize, Center.Y * CellSize, 0.0);

	// 셀마다 4방향 이웃 셀이 방 밖이면 그 경계가 개구부. 빌더가 그 경계에 문/벽을 놓는 식(CellA = 방 안, CellB = 방 밖)으로 자리를 구한다
	const FIntPoint Directions[] = { FIntPoint(0, -1), FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(-1, 0) };
	TArray<FTransform> Openings;
	for (int32 Y = Bounds.Min.Y; Y < Bounds.Max.Y; ++Y)
	{
		for (int32 X = Bounds.Min.X; X < Bounds.Max.X; ++X)
		{
			for (const FIntPoint& Direction : Directions)
			{
				FPMGResultConnector Connector;
				Connector.CellA = FIntPoint(X, Y);
				Connector.CellB = Connector.CellA + Direction;
				if (!Bounds.Contains(Connector.CellB))
				{
					Openings.Add(GetConnectorSpawnTransform(Connector, CellSize, Origin));
				}
			}
		}
	}
	return Openings;
}

APMGMapBuilder::APMGMapBuilder()
{
	// 완료 이벤트를 보낼 때까지만 Tick한다 (Generate에서 켜고, 끝나면 끈다)
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 레벨에 놓았을 때 위치를 가질 수 있도록 루트 컴포넌트를 둔다 (맵의 (0, 0) 셀 모서리 = 이 위치)
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// 구조 미리보기 선. DrawDebug* 함수가 월드에 그릴 때 쓰는 선 묶음 컴포넌트를 빌더가 따로 가져서, 다른 디버그 선과 섞이지 않고 이 빌더의 선만 지울 수 있다
	PreviewLines = CreateDefaultSubobject<ULineBatchComponent>(TEXT("PreviewLines"));
	PreviewLines->SetupAttachment(RootComponent);
}

int32 APMGMapBuilder::MakeRandomSeed()
{
	// 시드를 고르는 것뿐이라 시간 값을 써도 된다 (생성 코드는 항상 받은 시드만 쓴다. 테스트의 pmg.TestSeed 0과 같은 방식).
	// 맨 위 비트를 지워 0 이상으로 만든다 (Settings의 Seed에 옮겨 적기 쉽게)
	return static_cast<int32>(FPlatformTime::Cycles() & 0x7FFFFFFF);
}

void APMGMapBuilder::BeginPlay()
{
	Super::BeginPlay();
	if (bGenerateOnBeginPlay)
	{
		int32 Seed = Settings ? Settings->Seed : 0;   // Settings가 None이면 Generate가 실패 결과를 보낸다
		if (bUseRandomSeed)
		{
			Seed = MakeRandomSeed();
			UE_LOG(LogProcMapGen, Log, TEXT("%s: 무작위 시드 %d로 생성"), *GetName(), Seed);
		}
		Generate(Seed);
	}
}

void APMGMapBuilder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 게임 중에 빌더만 지워지면 스폰한 것도 지운다. 레벨 전환/종료 때는 월드가 모두 정리하므로 그대로 둔다
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		ReleaseSpawned();
	}
	if (ClassLoadHandle.IsValid())
	{
		ClassLoadHandle->CancelHandle();
		ClassLoadHandle.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void APMGMapBuilder::Generate(int32 Seed)
{
	Clear();

	if (!Settings)
	{
		Result.Seed = Seed;
		Result.FailReason = TEXT("Settings가 None");
		PMG::LogError(GetName() + TEXT(": ") + Result.FailReason);
	}
	else
	{
		// 생성에 실패하면 아무것도 스폰하지 않는다 (실패 사유는 Result.FailReason, 에러 로그는 생성기가 남김)
		Result = PMG::GenerateMap(*Settings, Seed);
		if (Result.bSuccess)
		{
			MakeSpawnRequests();
			// 프레임을 나눠 스폰할 때만 클래스를 미리 불러온다. 0이면 예전처럼 스폰하면서 그 자리에서 불러온다 (SpawnRequest의 LoadSynchronous)
			if (SpawnTimeBudgetMs > 0.f)
			{
				StartClassLoading();
			}
		}
	}

	// 완료 이벤트는 Tick에서 보낸다: 스폰과 레벨 로드를 기다려야 하고, 둘 다 없어도 다음 프레임에 보내
	// 이 함수를 BeginPlay에서 불렀을 때 다른 액터가 BeginPlay에서 바인딩해도 받을 수 있게 한다
	bGenerating = true;
	SetActorTickEnabled(true);

	// 기다릴 것(내려가는 레벨, 불러오는 클래스)이 없으면 지금 바로 스폰을 시작한다 (있으면 Tick에서)
	ContinueSpawning();
}

void APMGMapBuilder::Clear()
{
	// 내리기를 요청한 레벨은 레벨 스트리밍 갱신을 몇 번 거쳐(큰 레벨은 몇 프레임) 월드에서 빠진다.
	// 그 전에 같은 이름으로 다시 로드하면 엔진이 "이미 있는 이름"으로 거부하므로, 다음 생성이 기다릴 수 있게 따로 기억한다
	UnloadingLevels.Append(SpawnedLevels);
	ReleaseSpawned();

	// 불러오던 클래스는 더 기다리지 않는다 (요청을 취소하고 붙잡고 있던 클래스를 놓는다)
	if (ClassLoadHandle.IsValid())
	{
		ClassLoadHandle->CancelHandle();
		ClassLoadHandle.Reset();
	}

	Result = FPMGResult();
	SpawnErrors.Reset();
	SpawnRequests.Reset();
	NextSpawnIndex = 0;
	TotalRoomSpawnCount = 0;
	ProcessedRoomSpawnCount = 0;
	bGenerating = false;
	SetActorTickEnabled(false);
}

void APMGMapBuilder::ReleaseSpawned()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}
		// 지운 액터는 가비지 컬렉션 전까지 이름을 차지해서, 다음 생성에서 같은 이름("<빌더>_Room_12")을 쓰면 엔진이 다른 번호를 붙인다
		// (예: 방 0의 액터가 "_Room_20"이 됨). 그래서 지우기 전에 다른 이름으로 바꿔 원래 이름을 비워 둔다.
		// 엔진의 UChildActorComponent가 자식 액터를 지울 때 쓰는 방법과 같다 (가비지 컬렉션 중에는 이름을 바꿀 수 없음)
		if (!IsGarbageCollecting())
		{
			const FName DestroyedName = MakeUniqueObjectName(Actor->GetOuter(), Actor->GetClass(), TEXT("DESTROYED_PMG"));
			Actor->Rename(*DestroyedName.ToString(), nullptr, REN_DoNotDirty | REN_AllowPackageLinkerMismatch);
		}
		Actor->Destroy();
	}
	for (ULevelStreamingDynamic* StreamingLevel : SpawnedLevels)
	{
		if (StreamingLevel)
		{
			StreamingLevel->SetIsRequestingUnloadAndRemoval(true);
		}
	}
	SpawnedActors.Reset();
	SpawnedLevels.Reset();
	SpawnedLevelIsRoom.Reset();
}

void APMGMapBuilder::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bGenerating)
	{
		// 1. 앞의 레벨이 다 내려가고 클래스를 다 불러오면 이번 프레임 몫만큼 스폰 → 2. 모두 스폰했고 로드한 레벨이 모두 보이면(또는 실패하면) 완료
		ContinueSpawning();
		if (NextSpawnIndex >= SpawnRequests.Num() && CountLoadingLevels(false) == 0)
		{
			FinishGeneration();
		}
	}
}

float APMGMapBuilder::GetBuildProgress(int32& OutCompletedCount, int32& OutTotalCount) const
{
	// 스폰을 끝낸 수에서, 로드를 요청했지만 아직 보이지 않는 레벨을 뺀다
	OutTotalCount = SpawnRequests.Num();
	OutCompletedCount = NextSpawnIndex - CountLoadingLevels(false);
	return OutTotalCount > 0 ? static_cast<float>(OutCompletedCount) / OutTotalCount : 0.f;
}

void APMGMapBuilder::GetRoomBuildProgress(int32& OutCompletedRoomCount, int32& OutTotalRoomCount) const
{
	OutTotalRoomCount = TotalRoomSpawnCount;
	OutCompletedRoomCount = ProcessedRoomSpawnCount - CountLoadingLevels(true);
}

EPMGBuildState APMGMapBuilder::GetBuildState() const
{
	if (!bGenerating)
	{
		return EPMGBuildState::Idle;
	}
	if (NextSpawnIndex < SpawnRequests.Num())
	{
		// 스폰이 남았는데 시작하지 못하는 이유 순서대로 (ContinueSpawning과 같은 순서)
		if (HasUnloadingLevels())
		{
			return EPMGBuildState::WaitingForUnload;
		}
		if (IsLoadingClasses())
		{
			return EPMGBuildState::LoadingClasses;
		}
		return EPMGBuildState::Spawning;
	}
	return EPMGBuildState::LoadingLevels;
}

void APMGMapBuilder::MakeSpawnRequests()
{
	// 결과의 에셋 인덱스가 가리키는 리스트를 생성기와 같은 방법으로 만든다
	const FPMGAssetLists AssetLists = PMG::MakeAssetLists(*Settings, Settings->Validate());
	const FVector Origin = GetActorLocation();
	const float CellSize = Settings->CellSize;
	const FString BuilderName = GetName();
	SpawnRequests.Reset();
	NextSpawnIndex = 0;
	TotalRoomSpawnCount = 0;
	ProcessedRoomSpawnCount = 0;

	// 1. 방 (사용된 방 + 채움 방), 방 ID 순서. 에셋이 없는 방은 건너뛴다 (경고는 생성기가 이미 남김)
	for (const FPMGResultRoom& Room : Result.Rooms)
	{
		const FPMGAssetVariant* Variant = FindRoomVariantToSpawn(AssetLists, Room);
		if (!Variant)
		{
			continue;
		}
		FSpawnRequest& Request = SpawnRequests.AddDefaulted_GetRef();
		Request.Variant = *Variant;
		Request.Transform = PMG::GetRoomSpawnTransform(Room, CellSize, Origin);
		Request.ObjectName = FString::Printf(TEXT("%s_Room_%d"), *BuilderName, Room.Id);
		Request.Label = Room.bConnected ? FString::Printf(TEXT("방 %d (%s)"), Room.Id, *Room.RoomType.ToString()) : FString::Printf(TEXT("채움 방 %d"), Room.Id);
		Request.bRoom = true;
		TotalRoomSpawnCount++;
	}

	// 2. 문/벽, 커넥터 순서. 에셋이 없으면 건너뛴다 (문은 개구부만 열림, 벽은 막이 없음)
	for (int32 ConnectorIndex = 0; ConnectorIndex < Result.Connectors.Num(); ++ConnectorIndex)
	{
		const FPMGResultConnector& Connector = Result.Connectors[ConnectorIndex];
		const FPMGAssetVariant* Variant = FindConnectorVariantToSpawn(AssetLists, Connector);
		if (!Variant)
		{
			continue;
		}
		FSpawnRequest& Request = SpawnRequests.AddDefaulted_GetRef();
		Request.Variant = *Variant;
		Request.Transform = PMG::GetConnectorSpawnTransform(Connector, CellSize, Origin);
		Request.ObjectName = FString::Printf(TEXT("%s_Connector_%d"), *BuilderName, ConnectorIndex);
		Request.Label = FString::Printf(TEXT("커넥터 %d (%s)"), ConnectorIndex, Connector.Type == EPMGConnectorType::Door ? TEXT("문") : TEXT("벽"));
	}

	// 3. 입구/출구 마커 (Roadmap 3-7절): 방 중앙 바닥, 방 에셋과 같은 Transform. 클래스를 지정하지 않았으면 스폰하지 않는다.
	//    방 에셋이 스폰되지 않는 방(리스트가 비어 있음)이어도 마커는 스폰한다 (방 위치는 결과에 있으므로)
	auto AddMarker = [this, &Origin, CellSize, &BuilderName](const TSoftClassPtr<AActor>& MarkerClass, int32 RoomId, const TCHAR* NameSuffix, const TCHAR* Label)
	{
		if (MarkerClass.IsNull())
		{
			return;
		}
		FSpawnRequest& Request = SpawnRequests.AddDefaulted_GetRef();
		Request.Variant.ActorClass = MarkerClass;
		Request.Transform = PMG::GetRoomSpawnTransform(Result.Rooms[RoomId], CellSize, Origin);
		Request.ObjectName = BuilderName + NameSuffix;
		Request.Label = Label;
	};
	AddMarker(Settings->EntranceMarkerClass, Result.EntranceRoomId, TEXT("_EntranceMarker"), TEXT("입구 마커"));
	AddMarker(Settings->ExitMarkerClass, Result.ExitRoomId, TEXT("_ExitMarker"), TEXT("출구 마커"));
}

void APMGMapBuilder::StartClassLoading()
{
	// 아직 메모리에 없는 클래스만 모은다 (같은 클래스는 한 번). 이미 있는 클래스(엔진 클래스, 앞에서 쓴 방 BP)는 기다릴 필요가 없다
	TArray<FSoftObjectPath> ClassPaths;
	for (const FSpawnRequest& Request : SpawnRequests)
	{
		const TSoftClassPtr<AActor>& ActorClass = Request.Variant.ActorClass;
		if (!ActorClass.IsNull() && !ActorClass.Get())
		{
			ClassPaths.AddUnique(ActorClass.ToSoftObjectPath());
		}
	}
	// 에셋 매니저는 엔진이 시작할 때 만든다. 없으면 미리 불러오지 않고 스폰하면서 그 자리에서 불러온다 (SpawnRequest의 LoadSynchronous)
	if (ClassPaths.IsEmpty() || !UAssetManager::IsInitialized())
	{
		return;
	}
	// 엔진 공용 비동기 로더에 요청한다. 불러오는 동안에도 게임은 계속 프레임을 돌린다. 끝났는지는 Tick이 핸들로 본다
	ClassLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(ClassPaths));
}

bool APMGMapBuilder::HasUnloadingLevels() const
{
	// 월드의 스트리밍 레벨 목록에서 빠진 레벨은 더 기다리지 않는다 (엔진은 이 목록에서 같은 이름을 찾는다)
	const UWorld* World = GetWorld();
	return UnloadingLevels.ContainsByPredicate([World](const ULevelStreamingDynamic* StreamingLevel)
	{
		return StreamingLevel && World->GetStreamingLevels().Contains(StreamingLevel);
	});
}

bool APMGMapBuilder::IsLoadingClasses() const
{
	// 실패한 요청(없는 클래스)도 끝난 것이다. 불러오지 못한 클래스는 SpawnRequest의 LoadSynchronous도 실패해 SpawnErrors에 남는다
	return ClassLoadHandle.IsValid() && ClassLoadHandle->IsLoadingInProgress();
}

void APMGMapBuilder::ContinueSpawning()
{
	if (NextSpawnIndex >= SpawnRequests.Num())
	{
		return;
	}
	if (HasUnloadingLevels() || IsLoadingClasses())
	{
		return;
	}
	UnloadingLevels.Reset();   // 모두 월드에서 빠졌으므로 더 기억하지 않는다

	// 이번 프레임 몫: 시간 예산을 넘을 때까지 순서대로 스폰한다. 하나를 스폰한 뒤에 시간을 보므로 프레임마다 최소 1개는 스폰한다
	const double StartSeconds = FPlatformTime::Seconds();
	while (NextSpawnIndex < SpawnRequests.Num())
	{
		const FSpawnRequest& Request = SpawnRequests[NextSpawnIndex];
		SpawnRequest(Request);
		NextSpawnIndex++;
		if (Request.bRoom)
		{
			ProcessedRoomSpawnCount++;
		}

		const double ElapsedMilliseconds = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
		if (SpawnTimeBudgetMs > 0.f && ElapsedMilliseconds >= SpawnTimeBudgetMs)
		{
			break;
		}
	}
}

void APMGMapBuilder::SpawnRequest(const FSpawnRequest& Request)
{
	// 에셋 항목은 Level과 ActorClass 중 하나만 지정해야 한다 (Roadmap 3-1절).
	// 둘 다 있는 항목은 UPMGSettings::Validate()가 에러로 막아 생성 자체가 실패하므로 여기까지 올 수 없다 (스폰 목록은 생성에 성공했을 때만,
	// 검증한 리스트로 만든다). 둘 다 빈 항목은 경고라 생성이 진행되므로 여기서 건너뛴다
	const FPMGAssetVariant& Variant = Request.Variant;
	const bool bHasLevel = !Variant.Level.IsNull();
	const bool bHasActorClass = !Variant.ActorClass.IsNull();
	checkf(!(bHasLevel && bHasActorClass), TEXT("%s: Level과 ActorClass가 모두 지정된 항목 (UPMGSettings::Validate()가 막아야 함)"), *Request.Label);
	if (!bHasLevel && !bHasActorClass)
	{
		PMG::AddWarning(SpawnErrors, Request.Label + TEXT(": 에셋 항목에 Level과 ActorClass가 모두 비어 있음 → 스폰 생략"));
		return;
	}

	if (bHasLevel)
	{
		// 레벨: 동적 레벨 스트리밍으로 로드를 시작한다 (비동기, 끝났는지는 Tick이 본다).
		// 레벨 이름을 ObjectName으로 정해 서버와 클라이언트에서 같은 이름이 되게 한다 (레벨 안의 리플리케이트 액터를 같은 액터로 알아보도록)
		bool bLoadStarted = false;
		ULevelStreamingDynamic* StreamingLevel = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(this, Variant.Level, Request.Transform, bLoadStarted,
			Request.ObjectName);
		if (!StreamingLevel || !bLoadStarted)
		{
			PMG::AddWarning(SpawnErrors, Request.Label + FString::Printf(TEXT(": 레벨 로드를 시작하지 못함 (%s)"), *Variant.Level.ToString()));
			return;
		}
		SpawnedLevels.Add(StreamingLevel);
		SpawnedLevelIsRoom.Add(Request.bRoom);
		return;
	}

	// 액터: 클래스를 불러온다. 프레임을 나눠 스폰하면 StartClassLoading이 미리 불러 두어 바로 돌아온다 (SpawnTimeBudgetMs가 0이면 여기서 기다림)
	UClass* Class = Variant.ActorClass.LoadSynchronous();
	if (!Class)
	{
		PMG::AddWarning(SpawnErrors, Request.Label + FString::Printf(TEXT(": ActorClass를 불러오지 못함 (%s)"), *Variant.ActorClass.ToString()));
		return;
	}
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = FName(*Request.ObjectName);
	// 그 이름의 다른 오브젝트가 레벨에 있으면 엔진이 번호를 바꾼 이름을 붙인다 (빌더가 지운 액터는 ReleaseSpawned에서 이름을 비워 두므로 보통 일어나지 않음)
	SpawnParameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;   // 방/벽은 서로 맞닿으므로 겹쳐도 스폰
	AActor* Actor = GetWorld()->SpawnActor<AActor>(Class, Request.Transform, SpawnParameters);
	if (!Actor)
	{
		PMG::AddWarning(SpawnErrors, Request.Label + FString::Printf(TEXT(": 액터 스폰 실패 (%s)"), *Class->GetName()));
		return;
	}
	SpawnedActors.Add(Actor);
}

int32 APMGMapBuilder::CountLoadingLevels(bool bRoomsOnly) const
{
	int32 LoadingLevelCount = 0;
	for (int32 LevelIndex = 0; LevelIndex < SpawnedLevels.Num(); ++LevelIndex)
	{
		if (bRoomsOnly && !SpawnedLevelIsRoom[LevelIndex])
		{
			continue;
		}
		const ELevelStreamingState State = SpawnedLevels[LevelIndex]->GetLevelStreamingState();
		if (State != ELevelStreamingState::LoadedVisible && State != ELevelStreamingState::FailedToLoad)
		{
			LoadingLevelCount++;
		}
	}
	return LoadingLevelCount;
}

void APMGMapBuilder::FinishGeneration()
{
	for (const ULevelStreamingDynamic* StreamingLevel : SpawnedLevels)
	{
		if (StreamingLevel->GetLevelStreamingState() == ELevelStreamingState::FailedToLoad)
		{
			PMG::AddWarning(SpawnErrors, FString::Printf(TEXT("레벨 로드 실패: %s (원본 %s)"), *StreamingLevel->GetWorldAssetPackageName(),
				*StreamingLevel->PackageNameToLoad.ToString()));
		}
	}

	// 스폰이 끝났으니 미리 불러온 클래스를 붙잡고 있지 않는다 (스폰한 액터가 자기 클래스를 참조한다)
	ClassLoadHandle.Reset();

	bGenerating = false;
	SetActorTickEnabled(false);
	if (Result.bSuccess)
	{
		UE_LOG(LogProcMapGen, Log, TEXT("%s: 스폰 완료 (Seed %d, 액터 %d개, 레벨 %d개, 스폰 문제 %d개)"), *GetName(), Result.Seed, SpawnedActors.Num(), SpawnedLevels.Num(),
			SpawnErrors.Num());
	}
	OnGenerationFinished.Broadcast(Result, SpawnErrors);
}

void APMGMapBuilder::PreviewLayout()
{
	// 플레이할 때 BeginPlay가 쓰는 시드 (Settings가 None이면 PreviewWithSeed가 에러를 남긴다)
	PreviewWithSeed(Settings ? Settings->Seed : 0);
}

void APMGMapBuilder::PreviewRandomSeed()
{
	PreviewWithSeed(MakeRandomSeed());
}

void APMGMapBuilder::PreviewWithSeed(int32 Seed)
{
	if (!Settings)
	{
		PMG::LogError(GetName() + TEXT(": 미리보기 - Settings가 None"));
		ClearPreview();
		return;
	}

	// 플레이할 때 BeginPlay가 만드는 맵과 같은 함수 (성공/실패 로그는 생성기가 남긴다)
	PreviewSeed = Seed;
	UE_LOG(LogProcMapGen, Log, TEXT("%s: 미리보기 Seed %d"), *GetName(), Seed);
	PreviewResult = PMG::GenerateMap(*Settings, Seed);
	DrawPreview();
	if (!PreviewResult.bSuccess)
	{
		return;
	}

	// 커스텀 타입의 색은 선만 보고는 알 수 없으므로, 배정된 타입마다 색 이름을 로그에 남긴다
	TArray<FName> LoggedTypeNames;
	for (const FPMGCustomRoomType& CustomType : Settings->CustomTypes)
	{
		const FPreviewTypeColor* TypeColor = FindCustomTypePreviewColor(*Settings, CustomType.TypeName);
		if (LoggedTypeNames.Contains(CustomType.TypeName) || !TypeColor)
		{
			continue;
		}
		LoggedTypeNames.Add(CustomType.TypeName);
		int32 RoomCount = 0;
		for (const FPMGResultRoom& Room : PreviewResult.Rooms)
		{
			if (Room.bConnected && Room.RoomType == CustomType.TypeName)
			{
				RoomCount++;
			}
		}
		if (RoomCount > 0)
		{
			UE_LOG(LogProcMapGen, Log, TEXT("%s: 미리보기 방 타입 %s = %s (방 %d개)"), *GetName(), *CustomType.TypeName.ToString(), TypeColor->Name, RoomCount);
		}
	}
}

void APMGMapBuilder::ClearPreview()
{
	PreviewResult = FPMGResult();
	PreviewLines->Flush();
}

void APMGMapBuilder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 선은 그린 순간의 월드 위치에 남으므로, 빌더를 옮기면 지금 위치로 다시 그린다
	DrawPreview();
}

void APMGMapBuilder::DrawPreview()
{
	PreviewLines->Flush();
	if (!Settings || !PreviewResult.bSuccess)
	{
		return;
	}

	const float CellSize = Settings->CellSize;
	const FVector Origin = GetActorLocation();
	const float Thickness = CellSize * 0.02f;   // 선 굵기도 월드 단위(cm)라 셀 크기에 맞춘다
	const float LifeTime = -1.f;                // 음수 = 지울 때(Flush)까지 남음
	const uint8 DepthPriority = SDPG_World;

	// 높이: 같은 높이의 선은 서로 가리므로 방 < 문 < 루트 순서로 조금씩 띄운다 (Origin 위 cm)
	const float RoomHeight = 10.f;
	const float DoorHeight = 20.f;
	const float PrimaryRouteHeight = 30.f;
	const float SecondaryRouteHeight = 40.f;

	// 원색(RGB, CMY)은 커스텀 타입 상자에 쓰므로, 테두리와 기호는 흰색/회색, 문과 루트는 원색이 아닌 색
	const FColor UsedRoomColor(255, 255, 255);
	const FColor UnusedRoomColor(100, 100, 100);
	const FColor EntranceExitColor(255, 255, 255);
	const FColor DoorColor(255, 140, 0);
	const FColor PrimaryRouteColor(255, 120, 180);     // 분홍
	const FColor SecondaryRouteColor(100, 190, 255);   // 하늘색

	// 셀 좌표(셀 단위, 소수 가능) → 월드 위치
	auto CellToWorld = [&Origin, CellSize](const FVector2D& CellPoint, float Height)
	{
		return Origin + FVector(CellPoint.X * CellSize, CellPoint.Y * CellSize, Height);
	};

	// 셀 범위를 Inset(셀 단위)만큼 안으로 줄인 사각형. 맞닿은 두 방의 테두리가 한 선으로 겹치지 않게 줄인다
	auto DrawRoomRect = [this, &CellToWorld, RoomHeight, DepthPriority, LifeTime](const FIntRect& Bounds, double Inset, const FColor& Color, float LineThickness)
	{
		const FVector Corners[] =
		{
			CellToWorld(FVector2D(Bounds.Min.X + Inset, Bounds.Min.Y + Inset), RoomHeight),
			CellToWorld(FVector2D(Bounds.Max.X - Inset, Bounds.Min.Y + Inset), RoomHeight),
			CellToWorld(FVector2D(Bounds.Max.X - Inset, Bounds.Max.Y - Inset), RoomHeight),
			CellToWorld(FVector2D(Bounds.Min.X + Inset, Bounds.Max.Y - Inset), RoomHeight),
		};
		for (int32 CornerIndex = 0; CornerIndex < 4; ++CornerIndex)
		{
			PreviewLines->DrawLine(Corners[CornerIndex], Corners[(CornerIndex + 1) % 4], Color, DepthPriority, LineThickness, LifeTime);
		}
	};

	// 빌더가 문을 놓는 자리와 방향 (경계 중점, Yaw = CellA → CellB). 스폰과 같은 함수로 구한다
	auto GetDoorTransform = [CellSize, &Origin](const FIntPoint& CellA, const FIntPoint& CellB, float Height)
	{
		FPMGResultConnector DoorEdge;
		DoorEdge.CellA = CellA;
		DoorEdge.CellB = CellB;
		return PMG::GetConnectorSpawnTransform(DoorEdge, CellSize, Origin + FVector(0.0, 0.0, Height));
	};

	// 1. 방: 테두리 (사용 흰색, 미사용 회색. 크기는 테두리 모양으로 보임), 이중선 = 추가방,
	//    가운데 = 방 타입 (입구 원, 출구 X표, 커스텀 타입은 색 상자, Standard는 없음)
	const float MarkerRadius = CellSize * 0.15f;
	for (const FPMGResultRoom& Room : PreviewResult.Rooms)
	{
		if (!Room.bConnected)
		{
			DrawRoomRect(Room.Bounds, 0.05, UnusedRoomColor, Thickness * 0.5f);
			continue;
		}

		DrawRoomRect(Room.Bounds, 0.05, UsedRoomColor, Thickness);
		if (Room.bExtra)
		{
			DrawRoomRect(Room.Bounds, 0.12, UsedRoomColor, Thickness);
		}

		const FVector Center = CellToWorld(PMG::GetRectCenter(Room.Bounds), RoomHeight);
		if (Room.RoomType == PMG::RoomTypeEntrance())
		{
			// 원: 둘레를 짧은 선 16개로 나눠 그린다
			const int32 SegmentCount = 16;
			for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
			{
				const double StartAngle = 2.0 * PI * SegmentIndex / SegmentCount;
				const double EndAngle = 2.0 * PI * (SegmentIndex + 1) / SegmentCount;
				const FVector Start = Center + FVector(FMath::Cos(StartAngle), FMath::Sin(StartAngle), 0.0) * MarkerRadius;
				const FVector End = Center + FVector(FMath::Cos(EndAngle), FMath::Sin(EndAngle), 0.0) * MarkerRadius;
				PreviewLines->DrawLine(Start, End, EntranceExitColor, DepthPriority, Thickness * 1.5f, LifeTime);
			}
		}
		else if (Room.RoomType == PMG::RoomTypeExit())
		{
			// X표: 가운데를 지나는 대각선 두 개
			PreviewLines->DrawLine(Center + FVector(-MarkerRadius, -MarkerRadius, 0.0), Center + FVector(MarkerRadius, MarkerRadius, 0.0), EntranceExitColor, DepthPriority,
				Thickness * 1.5f, LifeTime);
			PreviewLines->DrawLine(Center + FVector(-MarkerRadius, MarkerRadius, 0.0), Center + FVector(MarkerRadius, -MarkerRadius, 0.0), EntranceExitColor, DepthPriority,
				Thickness * 1.5f, LifeTime);
		}
		else if (const FPreviewTypeColor* CustomTypeColor = FindCustomTypePreviewColor(*Settings, Room.RoomType))
		{
			const FVector MarkerExtent(MarkerRadius, MarkerRadius, 5.0);
			PreviewLines->DrawSolidBox(FBox(Center - MarkerExtent, Center + MarkerExtent), FTransform::Identity, CustomTypeColor->Color, DepthPriority, LifeTime);
		}
	}

	// 2. 문: 경계에 걸친 채운 화살표. 몸통(직사각형)은 RoomA 쪽, 촉(삼각형)은 RoomB 쪽 = 들어가는 방향. 촉의 밑변이 경계 위 = 문 자리.
	//    선으로 그린 화살표는 촉이 선 굵기에 묻혀 멀리서 방향이 안 보여서 면(메시)으로 그린다
	const double ArrowPartLength = CellSize * 0.2;    // 몸통 길이 = 촉 길이
	const double ShaftHalfWidth = CellSize * 0.04;
	const double HeadHalfWidth = CellSize * 0.125;
	for (const FPMGResultDoor& Door : PreviewResult.Doors)
	{
		const FTransform DoorTransform = GetDoorTransform(Door.CellA, Door.CellB, DoorHeight);
		const FVector Location = DoorTransform.GetLocation();
		const FVector Forward = DoorTransform.GetRotation().GetForwardVector();   // CellA → CellB
		const FVector Along = DoorTransform.GetRotation().GetRightVector();       // 경계를 따라가는 방향
		const TArray<FVector> Vertices =
		{
			// 몸통 (직사각형): 0 ~ 3, RoomA 쪽에서 경계까지
			Location - Forward * ArrowPartLength - Along * ShaftHalfWidth,
			Location - Forward * ArrowPartLength + Along * ShaftHalfWidth,
			Location + Along * ShaftHalfWidth,
			Location - Along * ShaftHalfWidth,
			// 촉 (삼각형): 4 ~ 6, 밑변은 경계 위, 꼭짓점은 RoomB 쪽
			Location - Along * HeadHalfWidth,
			Location + Along * HeadHalfWidth,
			Location + Forward * ArrowPartLength,
		};
		// 점 번호 3개 = 삼각형 하나 (몸통 2개 + 촉 1개). 삼각형은 점을 도는 방향으로 앞/뒷면이 정해지고 뒷면은 안 그려질 수 있어서,
		// 같은 삼각형을 점 순서만 뒤집어 한 번 더 넣어 위아래 어느 쪽에서 봐도 보이게 한다
		const TArray<int32> Indices =
		{
			0, 1, 2,   0, 2, 3,   4, 5, 6,
			2, 1, 0,   3, 2, 0,   6, 5, 4,
		};
		PreviewLines->DrawMesh(Vertices, Indices, DoorColor, DepthPriority, LifeTime);
	}

	// 3. 루트: 방 중심 → 두 방 사이의 문 → 다음 방 중심. 큰 방은 들어오는 문과 나가는 문이 달라서 방 중심끼리만 이으면 벽을 지나가는 것처럼 보인다.
	//    루트 2는 조금 비켜 그린다 (두 루트가 같이 지나는 구간에서 둘 다 보이게)
	auto DrawRoute = [&](const TArray<int32>& Route, const FColor& Color, float Height, const FVector& Offset)
	{
		TArray<FVector> Points;
		for (int32 RouteIndex = 0; RouteIndex < Route.Num(); ++RouteIndex)
		{
			const FPMGResultRoom& Room = PreviewResult.Rooms[Route[RouteIndex]];
			Points.Add(CellToWorld(PMG::GetRectCenter(Room.Bounds), Height) + Offset);
			if (RouteIndex + 1 < Route.Num())
			{
				// 루트에서 이어지는 두 방 사이에는 항상 문이 있다 (Phase 5)
				const FPMGResultDoor* Door = FindResultDoor(PreviewResult, Route[RouteIndex], Route[RouteIndex + 1]);
				if (Door)
				{
					Points.Add(GetDoorTransform(Door->CellA, Door->CellB, Height).GetLocation() + Offset);
				}
			}
		}
		for (int32 PointIndex = 0; PointIndex + 1 < Points.Num(); ++PointIndex)
		{
			PreviewLines->DrawLine(Points[PointIndex], Points[PointIndex + 1], Color, DepthPriority, Thickness, LifeTime);
		}
	};
	DrawRoute(PreviewResult.PrimaryRoute, PrimaryRouteColor, PrimaryRouteHeight, FVector::ZeroVector);
	DrawRoute(PreviewResult.SecondaryRoute, SecondaryRouteColor, SecondaryRouteHeight, FVector(CellSize * 0.06, CellSize * 0.06, 0.0));
}
