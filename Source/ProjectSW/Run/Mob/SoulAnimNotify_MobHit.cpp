#include "Run/Mob/SoulAnimNotify_MobHit.h"
#include "Run/Mob/SoulMobComponent.h"
#include "Utility/SoulGameplayTags.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

USoulAnimNotify_MobHit::USoulAnimNotify_MobHit()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(255, 80, 60);
#endif
}

FString USoulAnimNotify_MobHit::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("MobHit R%.0f x%.2f"), Radius, DamageScale);
}

FVector USoulAnimNotify_MobHit::ComputeOrigin(const USkeletalMeshComponent* MeshComp) const
{
	FVector Foot = MeshComp->GetComponentLocation();
	if (const ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner()))
	{
		const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		Foot = Capsule->GetComponentLocation() - FVector(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight());
	}

	// 몹 메시는 Yaw -90으로 붙어 메시 +Y가 앞이다
	FVector Forward = MeshComp->GetRightVector();
	Forward.Z = 0.f;
	Forward = Forward.GetSafeNormal();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Forward);

	// 정예로 키운 몹은 판정 위치도 같이 커진다. BP 기본 스케일(보스)은 이미 저작 값에 들어 있어 곱하지 않는다. 프리뷰는 1
	const USoulMobComponent* Mob = USoulMobComponent::Find(MeshComp->GetOwner());
	const float Scale = Mob ? Mob->GetSizeScale() : 1.f;
	return Foot + (FVector(0.f, 0.f, GroundHeight) + Forward * ForwardOffset + Side * SideOffset) * Scale;
}

void USoulAnimNotify_MobHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
		return;

	AActor* Owner = MeshComp->GetOwner();
	UWorld* World = MeshComp->GetWorld();

	// 에디터 프리뷰: 판정 구만 그린다
	if (World && !World->IsGameWorld())
	{
		DrawDebugSphere(World, ComputeOrigin(MeshComp), Radius, 16, FColor::Red, false, 0.5f);
		return;
	}

	if (!Owner || !UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
		return;

	FGameplayEventData Payload;
	Payload.EventTag = SoulTags::Events::Montages::Attacks::Melee;
	Payload.Instigator = Owner;
	Payload.OptionalObject = this;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, Payload.EventTag, Payload);
}
