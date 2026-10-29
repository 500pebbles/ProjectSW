// 


#include "SoulAnimNotifyState_MeleeTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Utility/SoulGameplayTags.h"
#include "Utility/SoulLog.h"

namespace
{
	// 판정선 디버그 표시. 콘솔에서 SW.Debug.MeleeTrace 1 로 켬.
	TAutoConsoleVariable<bool> CVarDebugMeleeTrace(
		TEXT("SW.Debug.MeleeTrace"),
		false,
		TEXT("Draw melee blade sweeps. 0: off, 1: on"));

	constexpr float DebugDrawTime = 1.f;

	// 소유자에게 ASC가 있을 때만 이벤트 전송. 에디터 미리보기 액터(ASC 없음)에서 에러 로그가 찍히는 것을 막음.
	void SendEventToOwner(AActor* Owner, const FGameplayEventData& Payload)
	{
		if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner)) return;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, Payload.EventTag, Payload);
	}
}

USoulAnimNotifyState_MeleeTrace::USoulAnimNotifyState_MeleeTrace()
{
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
}

void USoulAnimNotifyState_MeleeTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner) return;

	// 소켓 이름 오타 확인. 구간마다 1번만 알림.
	if (!MeshComp->DoesSocketExist(StartSocketName) || !MeshComp->DoesSocketExist(EndSocketName))
	{
		SW_LOG_WARNING("[MeleeTrace] Socket missing on %s: %s / %s", *GetNameSafe(MeshComp->GetSkeletalMeshAsset()), *StartSocketName.ToString(), *EndSocketName.ToString());
	}

	// 판정 구간 시작 알림. 받는 쪽(GA)이 맞은 대상 목록을 비움.
	FGameplayEventData Payload;
	Payload.EventTag = SoulTags::Events::Melee::TraceBegin;
	Payload.Instigator = Owner;
	SendEventToOwner(Owner, Payload);
}

void USoulAnimNotifyState_MeleeTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner) return;
	if (!MeshComp->DoesSocketExist(StartSocketName) || !MeshComp->DoesSocketExist(EndSocketName)) return;

	// Step 1 : 지금 칼날 선(손잡이 쪽 -> 칼끝)을 따라 구 스윕. 오브젝트 쿼리라 막히지 않고 겹친 것을 전부 반환.
	const FVector Start = MeshComp->GetSocketLocation(StartSocketName);
	const FVector End = MeshComp->GetSocketLocation(EndSocketName);
	const TArray<AActor*> ActorsToIgnore = { Owner };
	const EDrawDebugTrace::Type DrawDebugType = CVarDebugMeleeTrace.GetValueOnAnyThread() ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	TArray<FHitResult> HitResults;
	UKismetSystemLibrary::SphereTraceMultiForObjects(MeshComp, Start, End, Radius, ObjectTypes, false, ActorsToIgnore, DrawDebugType, HitResults, true, FLinearColor::Red, FLinearColor::Green, DebugDrawTime);

	// Step 2 : 한 액터의 캡슐과 메시가 같이 맞을 수 있으므로 이번 스윕 안에서는 액터당 1번만 전송.
	TArray<AActor*> SentActors;
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || SentActors.Contains(HitActor)) continue;
		SentActors.Add(HitActor);

		FGameplayEventData Payload;
		Payload.EventTag = SoulTags::Events::Melee::Hit;
		Payload.Instigator = Owner;
		Payload.Target = HitActor;
		Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(Hit);
		SendEventToOwner(Owner, Payload);
	}
}