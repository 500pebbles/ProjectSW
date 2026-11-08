#include "Run/Event/SoulRoomEvent_Teleport.h"
#include "Run/Flow/SoulFloorDirectorComponent.h"
#include "Map/SoulPackedLevelActor.h"
#include "Utility/SoulGameplayTags.h"
#include "Utility/SoulLog.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Build/PMGMapBuilder.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

void ASoulRoomEvent_Teleport::ApplyEvent_Implementation(APawn* User)
{
	const USoulFloorDirectorComponent* Director = USoulFloorDirectorComponent::Get(this);
	if (!Director || !User)
		return;

	const FPMGResult& Result = Director->GetLastResult();
	const int32 CurrentRoom = Director->GetRoomIdAt(User->GetActorLocation());

	TArray<int32> Candidates;
	TArray<float> Weights;
	float Total = 0.f;
	for (const FPMGResultRoom& Room : Result.Rooms)
	{
		if (!Room.bConnected || Room.Id == CurrentRoom || Room.Id == Result.ExitRoomId)
			continue;
		const ASoulPackedLevelActor* RoomActor = Director->FindRoomActor(Room.Id);
		if (!RoomActor)
			continue;
		float Weight = RoomActor->IsVisited() ? WeightVisited : (RoomActor->HasCombat() ? WeightUnvisitedCombat : WeightUnvisitedPeaceful);
		if (Room.Id == Result.EntranceRoomId)
		{
			Weight = WeightEntrance;
		}
		if (Weight <= 0.f)
			continue;
		Candidates.Add(Room.Id);
		Weights.Add(Weight);
		Total += Weight;
	}
	if (Candidates.IsEmpty())
	{
		SW_LOG_WARNING("[EVENT] teleport: 후보 방 없음");
		return;
	}

	float Pick = FMath::FRandRange(0.f, Total);
	int32 Target = Candidates.Last();
	for (int32 i = 0; i < Candidates.Num(); ++i)
	{
		Pick -= Weights[i];
		if (Pick <= 0.f)
		{
			Target = Candidates[i];
			break;
		}
	}

	// 대상 방 임의 문(개구부) 안쪽 LandingDepth. 개구부 Yaw는 방 안 → 밖
	const FPMGResultRoom& TargetRoom = Result.Rooms[Target];
	const FTransform RoomTransform = PMG::GetRoomSpawnTransform(TargetRoom, Director->GetCellSize(), Director->GetBuilderOrigin());
	const TArray<FTransform> Openings = PMG::GetRoomOpeningTransforms(TargetRoom.Shape, Director->GetCellSize(), RoomTransform.GetLocation());
	FVector Landing = RoomTransform.GetLocation();
	if (!Openings.IsEmpty())
	{
		const FTransform& Opening = Openings[FMath::RandRange(0, Openings.Num() - 1)];
		Landing = Opening.GetLocation() - Opening.GetRotation().GetForwardVector() * LandingDepth;
	}
	Landing.Z = RoomTransform.GetLocation().Z + User->GetDefaultHalfHeight() + LandingLift;
	SW_LOG("[EVENT] teleport room %d -> %d (%s)", CurrentRoom, Target, *TargetRoom.RoomType.ToString());

	APlayerController* PC = Cast<APlayerController>(User->GetController());
	if (PC && PC->PlayerCameraManager && FadeTime > 0.f)
	{
		PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, FadeTime, FLinearColor::Black, false, true);
		GetWorldTimerManager().SetTimer(FadeTimer, FTimerDelegate::CreateUObject(this, &ThisClass::DoTeleport, TWeakObjectPtr<APawn>(User), Landing), FadeTime, false);
	}
	else
	{
		DoTeleport(User, Landing);
	}
}

void ASoulRoomEvent_Teleport::DoTeleport(TWeakObjectPtr<APawn> WeakPawn, FVector Location)
{
	APawn* Pawn = WeakPawn.Get();
	if (!Pawn)
		return;

	Pawn->TeleportTo(Location, Pawn->GetActorRotation(), false, true);

	if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()); PC && PC->PlayerCameraManager && FadeTime > 0.f)
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeTime, FLinearColor::Black, false, false);
	}

	// 착지 직후 무적 (매복 첫 타 보호)
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn); ASC && LandingInvincibleTime > 0.f)
	{
		ASC->AddLooseGameplayTag(SoulTags::States::Invincible);
		TWeakObjectPtr<UAbilitySystemComponent> WeakASC = ASC;
		GetWorldTimerManager().SetTimer(InvincibleTimer, FTimerDelegate::CreateWeakLambda(this, [WeakASC]()
		{
			if (WeakASC.IsValid())
			{
				WeakASC->RemoveLooseGameplayTag(SoulTags::States::Invincible);
			}
		}), LandingInvincibleTime, false);
	}
}
