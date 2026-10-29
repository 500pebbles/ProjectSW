#include "Build/PMGRoomGuide.h"
#include "Build/PMGMapBuilder.h"
#include "Data/PMGSettings.h"
#include "Components/LineBatchComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

APMGRoomGuide::APMGRoomGuide()
{
	// 방 에셋을 만들 때만 쓰는 도구라 패키징한 게임에서는 빠지게 한다 (방 레벨에 저장해 두어도 게임에 들어가지 않음)
	bIsEditorOnlyActor = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	GuideLines = CreateDefaultSubobject<ULineBatchComponent>(TEXT("GuideLines"));
	GuideLines->SetupAttachment(RootComponent);
}

void APMGRoomGuide::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	GuideLines->Flush();
	// 플레이 중에는 그리지 않는다 (방 레벨을 스트리밍으로 불러온 PIE 화면에 선이 겹치지 않게)
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		return;
	}

	const float CellSize = Settings ? Settings->CellSize : GetDefault<UPMGSettings>()->CellSize;
	const FVector Pivot = GetActorLocation();
	const float Thickness = CellSize * 0.01f;
	const float LifeTime = -1.f;   // 음수 = 지울 때(Flush)까지 남음
	const uint8 DepthPriority = SDPG_Foreground;   // 방 메시 앞에 그려 벽 속에 묻힌 선도 보이게

	const FColor RoomColor(255, 255, 255);
	const FColor CellLineColor(150, 150, 150);
	const FColor OpeningColor(0, 255, 0);
	const FColor PivotColor(255, 220, 0);

	// 방 크기 (칸 수). 2칸은 가로 = X축 2칸
	const FIntPoint RoomSize = PMG::GetShapeCellCount(Shape);
	const FVector HalfSize(RoomSize.X * CellSize * 0.5, RoomSize.Y * CellSize * 0.5, GuideHeight * 0.5);

	// 1. 방 영역 상자 (바닥 ~ GuideHeight)
	GuideLines->DrawBox(Pivot + FVector(0.0, 0.0, HalfSize.Z), HalfSize, RoomColor, LifeTime, DepthPriority, Thickness);

	// 2. 방 안의 셀 경계 (바닥). 2칸/4칸 방에서 셀이 어떻게 나뉘는지
	for (int32 X = 1; X < RoomSize.X; ++X)
	{
		const double LineX = -HalfSize.X + X * CellSize;
		GuideLines->DrawLine(Pivot + FVector(LineX, -HalfSize.Y, 0.0), Pivot + FVector(LineX, HalfSize.Y, 0.0), CellLineColor, DepthPriority, Thickness, LifeTime);
	}
	for (int32 Y = 1; Y < RoomSize.Y; ++Y)
	{
		const double LineY = -HalfSize.Y + Y * CellSize;
		GuideLines->DrawLine(Pivot + FVector(-HalfSize.X, LineY, 0.0), Pivot + FVector(HalfSize.X, LineY, 0.0), CellLineColor, DepthPriority, Thickness, LifeTime);
	}

	// 3. 개구부: 빌더가 문/벽을 놓는 자리와 같은 함수로 구한다. 세로선 = 자리, 바닥 화살표 = 방 밖 방향 (문/벽 에셋의 +X가 이 방향 또는 반대 방향)
	for (const FTransform& Opening : PMG::GetRoomOpeningTransforms(Shape, CellSize, Pivot))
	{
		const FVector Location = Opening.GetLocation();
		const FVector Outward = Opening.GetRotation().GetForwardVector();
		GuideLines->DrawLine(Location, Location + FVector(0.0, 0.0, GuideHeight), OpeningColor, DepthPriority, Thickness * 2.f, LifeTime);
		GuideLines->DrawDirectionalArrow(Location - Outward * CellSize * 0.1, Location + Outward * CellSize * 0.15, CellSize * 0.05f, OpeningColor, LifeTime, DepthPriority,
			Thickness);
	}

	// 4. 피벗과 +X 방향 (에셋의 앞쪽. 2칸 에셋은 이 방향으로 2칸)
	GuideLines->DrawDirectionalArrow(Pivot, Pivot + FVector(CellSize * 0.25, 0.0, 0.0), CellSize * 0.25f, PivotColor, LifeTime, DepthPriority, Thickness);
}
