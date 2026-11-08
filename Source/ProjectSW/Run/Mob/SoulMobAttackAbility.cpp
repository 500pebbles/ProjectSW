#include "Run/Mob/SoulMobAttackAbility.h"
#include "Run/Mob/SoulAnimNotify_MobHit.h"
#include "Run/Mob/SoulMobComponent.h"
#include "Run/Mob/SoulMobProjectile.h"
#include "Character/SoulCharacterEnemy.h"
#include "GAS/Utility/SoulAbilitySystemLibrary.h"
#include "Interface/SoulCombatInterface.h"
#include "Utility/SoulGameplayTags.h"
#include "Utility/SoulLog.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarSoulMobDebugHit(TEXT("soul.Mob.DebugHit"), 0, TEXT("1이면 몹 공격 판정 구를 그린다"));

USoulMobAttackAbility::USoulMobAttackAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 몹 피해는 CT_Damage_Mob과 배율만 따른다 (공격력 성장은 플레이어 전용)
	AttackPowerReference = 0.f;

	// BTTask_Attack이 Abilities.Attack 태그로 켠다
	FGameplayTagContainer Tags;
	Tags.AddTag(SoulTags::Abilities::Attack);
	SetAssetTags(Tags);
}

bool USoulMobAttackAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
		return false;

	// BT가 0.2초마다 두드려도 간격 전에는 조용히 실패한다
	const USoulMobComponent* Mob = USoulMobComponent::Find(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
	return !Mob || Mob->CanAttackNow();
}

void USoulMobAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ASoulCharacterEnemy* Enemy = Cast<ASoulCharacterEnemy>(GetAvatarActorFromActorInfo());
	USoulMobComponent* Mob = USoulMobComponent::Find(Enemy);
	AActor* Target = Enemy ? ISoulCombatInterface::Execute_GetCombatTarget(Enemy) : nullptr;
	UAnimMontage* Montage = Mob ? Mob->ChooseAttack(Target) : nullptr;

	if (!Enemy || !Montage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveMontage = Montage;
	ActiveTarget = Target;
	bFirstHitDone = false;
	Mob->NotifyAttackStarted(Montage);

	// 루트모션이 없는 클립이라 방향은 AI 포커스로 맞추고, 공격 중에는 미끄러지지 않게 세운다
	if (AAIController* AI = Cast<AAIController>(Enemy->GetController()))
	{
		AI->SetFocus(Target);
	}
	Enemy->GetCharacterMovement()->MaxWalkSpeed = 0.f;

	UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SoulTags::Events::Montages::Attacks::Melee, nullptr, false, true);
	WaitHit->EventReceived.AddDynamic(this, &ThisClass::HandleHitEvent);
	WaitHit->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* Play = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, 1.f, NAME_None, true);
	Play->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageEnded);
	Play->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageEnded);
	Play->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageEnded);
	Play->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageEnded);
	Play->ReadyForActivation();

	SW_LOG("[MOB] attack %s montage=%s target=%s dist=%.0f", *Enemy->GetName(), *Montage->GetName(), Target ? *Target->GetName() : TEXT("null"), Target ? FVector::Dist2D(Enemy->GetActorLocation(), Target->GetActorLocation()) : -1.f);
}

void USoulMobAttackAbility::HandleHitEvent(FGameplayEventData Payload)
{
	ASoulCharacterEnemy* Enemy = Cast<ASoulCharacterEnemy>(GetAvatarActorFromActorInfo());
	if (!Enemy || !Enemy->HasAuthority())
		return;

	if (!bFirstHitDone)
	{
		bFirstHitDone = true;
		if (AAIController* AI = Cast<AAIController>(Enemy->GetController()))
		{
			AI->ClearFocus(EAIFocusPriority::Gameplay);
		}
	}

	const USoulAnimNotify_MobHit* HitNotify = Cast<USoulAnimNotify_MobHit>(Payload.OptionalObject);
	FVector Origin;
	float Radius;
	float NotifyScale = 1.f;
	if (HitNotify)
	{
		Origin = HitNotify->ComputeOrigin(Enemy->GetMesh());
		Radius = HitNotify->Radius;
		NotifyScale = HitNotify->DamageScale;
	}
	else
	{
		const float HalfHeight = Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Origin = Enemy->GetActorLocation() + Enemy->GetActorForwardVector() * FallbackForwardOffset - FVector(0.f, 0.f, HalfHeight * 0.5f);
		Radius = FallbackRadius;
	}

	if (CVarSoulMobDebugHit.GetValueOnGameThread() > 0)
	{
		DrawDebugSphere(Enemy->GetWorld(), Origin, Radius, 16, FColor::Red, false, 1.f);
	}

	TArray<AActor*> Overlapping;
	USoulAbilitySystemLibrary::GetActorsWithinRadius(Enemy, Overlapping, TArray<AActor*>{ Enemy }, Radius, Origin);

	const USoulMobComponent* Mob = USoulMobComponent::Find(Enemy);
	const float Multiplier = NotifyScale * (Mob ? Mob->GetDamageMultiplier(ActiveMontage) : 1.f);

	const USoulMobComponent* MobForScale = USoulMobComponent::Find(Enemy);
	const float ShotMultiplier = NotifyScale * (MobForScale ? MobForScale->GetDamageMultiplier(ActiveMontage) : 1.f);

	// 원거리: 판정 원점에서 대상의 지금 위치로 쏜다 (선행 없음)
	if (HitNotify && HitNotify->ProjectileClass)
	{
		const AActor* Aim = ActiveTarget.Get();
		const FVector Direction = Aim ? (Aim->GetActorLocation() - Origin).GetSafeNormal() : Enemy->GetActorForwardVector();
		const FTransform SpawnTransform(Direction.Rotation(), Origin);
		ASoulMobProjectile* Projectile = Enemy->GetWorld()->SpawnActorDeferred<ASoulMobProjectile>(HitNotify->ProjectileClass, SpawnTransform, Enemy, Enemy, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Projectile)
		{
			Projectile->DamageEffectSpecHandle = MakeDamageSpec(ShotMultiplier);
			Projectile->FinishSpawning(SpawnTransform);
			SW_LOG("[MOB] shot %s x%.2f", *Enemy->GetName(), ShotMultiplier);
		}
		return;
	}

	const AActor* CombatTarget = ActiveTarget.Get();
	SW_LOG("[MOB] hit-event %s notify=%d radius=%.0f overlaps=%d targetDist=%.0f", *Enemy->GetName(), HitNotify != nullptr, Radius, Overlapping.Num(),
		CombatTarget ? FVector::Dist(Origin, CombatTarget->GetActorLocation()) : -1.f);

	for (AActor* Target : Overlapping)
	{
		if (!IsValid(Target) || USoulAbilitySystemLibrary::AreActorsAllies(Enemy, Target))
			continue;

		const UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (!TargetASC || TargetASC->HasMatchingGameplayTag(SoulTags::States::Dead) || TargetASC->HasMatchingGameplayTag(SoulTags::States::Invincible))
			continue;

		ApplyMeleeDamage(Target, Multiplier);
		SW_LOG("[MOB] hit %s -> %s x%.2f", *Enemy->GetName(), *Target->GetName(), Multiplier);
	}
}

FGameplayEffectSpecHandle USoulMobAttackAbility::MakeDamageSpec(float Multiplier) const
{
	// ApplyMeleeDamage와 같은 곱으로 Spec만 만든다 (탄이 맞을 때 적용)
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Avatar);
	if (!SourceASC || !DamageEffectClass)
		return FGameplayEffectSpecHandle();

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(Avatar);
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), Context);
	if (Spec.IsValid())
	{
		for (const TTuple<FGameplayTag, FScalableFloat>& Pair : DamageTypes)
		{
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(Spec, Pair.Key, Pair.Value.GetValueAtLevel(GetAbilityLevel()) * Multiplier);
		}
	}
	return Spec;
}

void USoulMobAttackAbility::HandleMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USoulMobAttackAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ASoulCharacterEnemy* Enemy = Cast<ASoulCharacterEnemy>(GetAvatarActorFromActorInfo()))
	{
		if (AAIController* AI = Cast<AAIController>(Enemy->GetController()))
		{
			AI->ClearFocus(EAIFocusPriority::Gameplay);
		}
		Enemy->GetCharacterMovement()->MaxWalkSpeed = Enemy->bHitReacting ? 0.f : Enemy->BaseWalkSpeed;
		// 간격은 공격이 끝난 때부터 잰다 (몽타주를 고르지 못한 시도는 공격이 아니다)
		if (USoulMobComponent* Mob = USoulMobComponent::Find(Enemy); Mob && ActiveMontage)
		{
			Mob->NotifyAttackEnded();
		}
	}
	ActiveMontage = nullptr;
	ActiveTarget.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
