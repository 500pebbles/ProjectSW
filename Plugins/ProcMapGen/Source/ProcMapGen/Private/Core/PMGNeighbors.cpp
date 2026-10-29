#include "Core/PMGNeighbors.h"

void PMG::BuildNeighbors(FPMGLayout& Layout)
{
	// 상하좌우 4방향 (대각선은 이웃이 아니다): +X, +Y, -X, -Y 순서
	const FIntPoint Directions[] = { FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(-1, 0), FIntPoint(0, -1) };

	// 방마다 자기 셀의 상하좌우 셀을 보고, 다른 방의 셀이면 그 방을 이웃으로 기록한다
	for (FPMGRoom& Room : Layout.Rooms)
	{
		Room.Neighbors.Reset();

		for (int32 Y = Room.Bounds.Min.Y; Y < Room.Bounds.Max.Y; ++Y)
		{
			for (int32 X = Room.Bounds.Min.X; X < Room.Bounds.Max.X; ++X)
			{
				const FIntPoint Cell(X, Y);
				for (const FIntPoint& Direction : Directions)
				{
					const FIntPoint OtherCell = Cell + Direction;
					if (!Layout.IsInsideMap(OtherCell))
					{
						continue;   // 맵 외곽
					}

					const int32 OtherRoomId = Layout.GetRoomIdAt(OtherCell);
					if (OtherRoomId == Room.Id)
					{
						continue;   // 같은 방 안쪽 (2칸/4칸 방)
					}

					// 큰 방은 같은 이웃과 셀 두 개로 맞닿을 수 있으므로 중복 없이 넣는다
					Room.Neighbors.AddUnique(OtherRoomId);
				}
			}
		}

		// 발견한 순서(셀 순서, 방향 순서)로 들어가 있으므로 오름차순으로 정렬해 둔다
		Room.Neighbors.Sort();
	}
}
