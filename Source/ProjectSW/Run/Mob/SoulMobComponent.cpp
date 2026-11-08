#include "Run/Mob/SoulMobComponent.h"
#include "Run/Mob/SoulMobEncounterSubsystem.h"
#include "Run/Mob/SoulMobPatternInterface.h"
#include "Character/SoulCharacterEnemy.h"
#include "Game/SoulGameModeBase.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "GAS/Data/SoulEnemyTypeInfo.h"
#include "Interface/SoulCombatInterface.h"
#include "Utility/SoulGameplayTags.h"
#include "Utility/SoulLog.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

USoulMobComponent::USoulMobComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

USoulMobComponent* USoulMobComponent::Find(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<USoulMobComponent>() : nullptr;
}

ASoulCharacterEnemy* USoulMobComponent::GetEnemy() const
{
	return Cast<ASoulCharacterEnemy>(GetOwner());
}

void USoulMobComponent::InitializeComponent()
{
	Super::InitializeComponent();

	ASoulCharacterEnemy* Enemy = GetEnemy();
	UWorld* World = GetWorld();
	if (!Enemy || !World || !World->IsGameWorld())
		return;

	// 새 EnemyType은 런 GameMode의 DA에만 있다. 팀 GameMode에서 스폰되면 FindChecked로 죽으므로 여기서 막는다
	const ASoulGameModeBase* GameMode = Cast<ASoulGameModeBase>(UGameplayStatics::GetGameMode(this));
	const USoulEnemyTypeInfo* TypeInfo = GameMode ? GameMode->EnemyTypeInfo.Get() : nullptr;
	if (!TypeInfo || !TypeInfo->EnemyDescription.Contains(Enemy->GetEnemyType()))
	{
		SW_LOG_ERROR("[MOB] %s: EnemyType %d가 GameMode의 EnemyTypeInfo에 없음. 런 GameMode에서만 스폰할 수 있음. 제거", *Enemy->GetName(), static_cast<int32>(Enemy->GetEnemyType()));
		Enemy->Destroy();
		return;
	}

	int32 Level = 0;
	int32 Score = ScoreValue;
	if (PendingLevel > 0)
	{
		Level = PendingLevel;
	}
	else if (LevelOverride > 0)
	{
		Level = LevelOverride;
	}
	else if (USoulMobEncounterSubsystem* Encounter = World->GetSubsystem<USoulMobEncounterSubsystem>())
	{
		Level = Encounter->ResolveLevel(Enemy->GetActorLocation(), Role, Score);
	}

	if (Level > 0)
	{
		Enemy->SetLevelForSpawn(Level);
	}
	ScoreValue = Score;

	// 강화 몹(정예): 보상과 크기는 여기서, 체력은 기본 속성이 들어간 뒤(ApplyToughStats)
	if (USoulMobEncounterSubsystem* Encounter = World->GetSubsystem<USoulMobEncounterSubsystem>(); Encounter && PendingLevel <= 0 && Encounter->ClaimTough(Enemy->GetActorLocation(), Role))
	{
		bTough = true;
		ToughRules = Encounter->GetToughRules();
		const int32 BaseScore = ScoreValue;
		const float BaseExpWeight = ExpWeight;
		ScoreValue = FMath::RoundToInt(ScoreValue * ToughRules.ScoreScale);
		ExpWeight *= ToughRules.ExpScale;
		Enemy->SetActorScale3D(Enemy->GetActorScale3D() * ToughRules.MeshScale);
		// 오버레이는 처음 그려지기 전에 건다. 보이던 메시에 나중에 걸면 쿡된 DX12에서 오버레이 PSO를 만드는 동안 메시가 몇 프레임 사라질 수 있다
		const int32 Overlaid = ApplyToughOverlay(true);
		SW_LOG("[MOB] tough pick %s score %d -> %d expWeight %.1f -> %.1f overlay=%d", *Enemy->GetName(), BaseScore, ScoreValue, BaseExpWeight, ExpWeight, Overlaid);
	}

	Enemy->GetCharacterMovement()->MaxWalkSpeed = Enemy->BaseWalkSpeed;
}

void USoulMobComponent::BeginPlay()
{
	Super::BeginPlay();

	ASoulCharacterEnemy* Enemy = GetEnemy();
	if (!Enemy || !GetWorld()->IsGameWorld() || !IsValid(Enemy))
		return;

	Enemy->OnDied.AddUObject(this, &ThisClass::HandleDied);

	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (ASC && bBlockHitReact)
	{
		ASC->BlockAbilitiesWithTags(FGameplayTagContainer(SoulTags::Combats::HitReact));
	}
	if (ASC && !Summons.IsEmpty())
	{
		SummonDone.Init(false, Summons.Num());
		ASC->GetGameplayAttributeValueChangeDelegate(USoulAttributeSet::GetHealthAttribute()).AddUObject(this, &ThisClass::HandleHealthChanged);
	}

	// 고정 좌표가 소품에 막힐 수 있어 NavMesh 위로 옮긴다
	if (!bStationary)
	{
		if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			FNavLocation NavLocation;
			const float HalfHeight = Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			if (NavSys->ProjectPointToNavigation(Enemy->GetActorLocation(), NavLocation, FVector(400.f, 400.f, 600.f)))
			{
				Enemy->SetActorLocation(NavLocation.Location + FVector(0.f, 0.f, HalfHeight), false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
	}

	if (bTough)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::ApplyToughStats));
	}

	if (SpawnMontage)
	{
		UAnimInstance* AnimInstance = Enemy->GetMesh()->GetAnimInstance();
		const float Length = AnimInstance ? AnimInstance->Montage_Play(SpawnMontage) : 0.f;
		if (Length > 0.f)
		{
			if (const AAIController* AI = Cast<AAIController>(Enemy->GetController()); AI && AI->GetBrainComponent())
			{
				AI->GetBrainComponent()->PauseLogic(TEXT("MobSpawn"));
			}
			GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &ThisClass::FinishSpawn, Length, false);
			return;
		}
	}

	FinishSpawn();
}

void USoulMobComponent::ApplyToughStats()
{
	ASoulCharacterEnemy* Enemy = GetEnemy();
	UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!ASC || bDead)
		return;

	// 레벨 기본값 위에 곱한다. 현재값 기준으로 기본값을 올려 다른 가산 효과가 있어도 현재값이 배율만큼 된다
	const FGameplayAttribute MaxHealthAttr = USoulAttributeSet::GetMaxHealthAttribute();
	const FGameplayAttribute HealthAttr = USoulAttributeSet::GetHealthAttribute();
	const float OldMax = ASC->GetNumericAttribute(MaxHealthAttr);
	ASC->SetNumericAttributeBase(MaxHealthAttr, ASC->GetNumericAttributeBase(MaxHealthAttr) + OldMax * (ToughRules.HealthScale - 1.f));
	const float NewMax = ASC->GetNumericAttribute(MaxHealthAttr);
	// 늘어난 만큼만 더한다. 스폰 직후 받은 피해는 남는다
	ASC->SetNumericAttributeBase(HealthAttr, ASC->GetNumericAttributeBase(HealthAttr) + (NewMax - OldMax));

	const UCapsuleComponent* Capsule = Enemy->GetCapsuleComponent();
	SW_LOG("[MOB] tough %s lv=%d maxHealth %.0f -> %.0f health=%.0f scale=%.2f capsule r=%.0f hh=%.0f score=%d expWeight=%.1f",
		*Enemy->GetName(), Enemy->GetCharacterLevel(), OldMax, NewMax, ASC->GetNumericAttribute(HealthAttr),
		Enemy->GetActorScale3D().X, Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight(), ScoreValue, ExpWeight);
}

int32 USoulMobComponent::ApplyToughOverlay(bool bOn)
{
	ASoulCharacterEnemy* Enemy = GetEnemy();
	if (!Enemy)
		return 0;
	if (bOn && !ToughMID && ToughRules.OverlayMaterial)
	{
		ToughMID = UMaterialInstanceDynamic::Create(ToughRules.OverlayMaterial, this);
		ToughMID->SetVectorParameterValue(TEXT("Color"), ToughRules.OverlayColor);
		ToughMID->SetScalarParameterValue(TEXT("Intensity"), ToughRules.OverlayIntensity);
		ToughMID->SetScalarParameterValue(TEXT("FresnelExponent"), ToughRules.OverlayFresnelExponent);
		ToughMID->SetScalarParameterValue(TEXT("BaseGlow"), ToughRules.OverlayBaseGlow);
	}
	if (!ToughMID)
		return 0;

	int32 Count = 0;
	TArray<USkeletalMeshComponent*> Meshes;
	Enemy->GetComponents(Meshes);
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (!Mesh)
			continue;
		if (bOn && Mesh->IsVisible() && !Mesh->GetOverlayMaterial())
		{
			// 팀이 이미 오버레이를 쓰는 메시는 건드리지 않는다
			Mesh->SetOverlayMaterial(ToughMID);
			++Count;
		}
		else if (!bOn && Mesh->GetOverlayMaterial() == ToughMID)
		{
			Mesh->SetOverlayMaterial(nullptr);
			++Count;
		}
	}
	return Count;
}

void USoulMobComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	Super::EndPlay(EndPlayReason);
}

void USoulMobComponent::FinishSpawn()
{
	ASoulCharacterEnemy* Enemy = GetEnemy();
	if (!Enemy || bDead)
		return;

	if (const AAIController* AI = Cast<AAIController>(Enemy->GetController()); AI && AI->GetBrainComponent())
	{
		AI->GetBrainComponent()->ResumeLogic(TEXT("MobSpawn"));
	}
	OnMobSpawnFinished.Broadcast(Enemy);
}

// ---------- 공격 ----------

bool USoulMobComponent::CanAttackNow() const
{
	const USoulMobEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<USoulMobEncounterSubsystem>();
	const float Scale = Encounter ? Encounter->GetAttackIntervalScale() : 1.f;
	return !bDead && GetWorld()->GetTimeSeconds() - LastAttackTime >= AttackInterval * Scale;
}

const FSoulMobAttackRule* USoulMobComponent::FindRule(const UAnimMontage* Montage) const
{
	return AttackRules.FindByPredicate([Montage](const FSoulMobAttackRule& Rule) { return Rule.Montage == Montage; });
}

UAnimMontage* USoulMobComponent::ChooseAttack(AActor* Target)
{
	ASoulCharacterEnemy* Enemy = GetEnemy();
	if (!Enemy || !Target)
		return nullptr;

	const float Distance = FVector::Dist2D(Enemy->GetActorLocation(), Target->GetActorLocation());

	if (Enemy->GetClass()->ImplementsInterface(USoulMobPatternInterface::StaticClass()))
	{
		return ISoulMobPatternInterface::Execute_ChooseAttackMontage(Enemy, Target, Distance);
	}

	const double Now = GetWorld()->GetTimeSeconds();
	TArray<UAnimMontage*> Candidates;
	TArray<float> Weights;
	float Total = 0.f;

	for (const FTaggedMontage& Tagged : ISoulCombatInterface::Execute_GetAttackMontages(Enemy))
	{
		UAnimMontage* Montage = Tagged.Montage;
		if (!Montage)
			continue;

		const FSoulMobAttackRule* Rule = FindRule(Montage);
		const float Weight = Rule ? Rule->Weight : 1.f;
		if (Weight <= 0.f)
			continue;
		if (Rule && Rule->MinRange > 0.f && Distance < Rule->MinRange)
			continue;
		if (Rule && Rule->MaxRange > 0.f && Distance > Rule->MaxRange)
			continue;
		if (const double* Next = NextUseTime.Find(Montage); Next && Now < *Next)
			continue;

		Candidates.Add(Montage);
		Weights.Add(Weight);
		Total += Weight;
	}

	if (Candidates.IsEmpty())
		return nullptr;

	float Pick = FMath::FRandRange(0.f, Total);
	for (int32 i = 0; i < Candidates.Num(); ++i)
	{
		Pick -= Weights[i];
		if (Pick <= 0.f)
			return Candidates[i];
	}
	return Candidates.Last();
}

void USoulMobComponent::NotifyAttackEnded()
{
	LastAttackTime = GetWorld()->GetTimeSeconds();
}

void USoulMobComponent::NotifyAttackStarted(UAnimMontage* Montage)
{
	LastAttackTime = GetWorld()->GetTimeSeconds();
	if (const FSoulMobAttackRule* Rule = FindRule(Montage); Rule && Rule->Cooldown > 0.f)
	{
		NextUseTime.Add(Montage, LastAttackTime + Rule->Cooldown);
	}
}

float USoulMobComponent::GetDamageMultiplier(UAnimMontage* Montage) const
{
	const FSoulMobAttackRule* Rule = FindRule(Montage);
	const USoulMobEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<USoulMobEncounterSubsystem>();
	return (Rule ? Rule->DamageScale : 1.f) * BaseDamageScale * (Encounter ? Encounter->GetDamageScale() : 1.f);
}

// ---------- 소환 ----------

void USoulMobComponent::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	const ASoulCharacterEnemy* Enemy = GetEnemy();
	const UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!ASC || bDead)
		return;

	const float MaxHealth = ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute());
	if (MaxHealth <= 0.f)
		return;

	const float Ratio = Data.NewValue / MaxHealth;
	for (int32 i = 0; i < Summons.Num(); ++i)
	{
		if (!SummonDone[i] && Data.NewValue > 0.f && Ratio <= Summons[i].HealthPercent)
		{
			SummonDone[i] = true;
			SpawnSummon(Summons[i]);
		}
	}
}

void USoulMobComponent::SpawnSummon(const FSoulMobSummon& Summon)
{
	ASoulCharacterEnemy* Enemy = GetEnemy();
	if (!Summon.Class || !Enemy)
		return;

	const FTransform Transform(Enemy->GetActorRotation(), Enemy->GetActorTransform().TransformPosition(Summon.LocalOffset));
	ASoulCharacterEnemy* Spawned = GetWorld()->SpawnActorDeferred<ASoulCharacterEnemy>(Summon.Class, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Spawned)
		return;

	if (USoulMobComponent* SummonMob = Find(Spawned); SummonMob && Summon.Level > 0)
	{
		SummonMob->SetPendingLevel(Summon.Level);
	}
	UGameplayStatics::FinishSpawningActor(Spawned, Transform);
	SpawnedSummons.Add(Spawned);
	SW_LOG("[MOB] %s summon %s", *Enemy->GetName(), *Spawned->GetName());
}

// ---------- 사망 ----------

void USoulMobComponent::HandleDied(AActor* DeadActor)
{
	if (bDead)
		return;
	bDead = true;

	ASoulCharacterEnemy* Enemy = GetEnemy();
	if (!Enemy)
		return;

	SW_LOG("[MOB] died %s level=%d", *Enemy->GetName(), Enemy->GetCharacterLevel());

	// 정예 붉은 기운은 죽으면 바로 뗀다 (시체가 남는 동안 정예처럼 보이지 않게)
	if (bTough)
	{
		SW_LOG("[MOB] tough overlay off %s meshes=%d", *Enemy->GetName(), ApplyToughOverlay(false));
	}

	if (!bRagdollOnDeath)
	{
		// 팀 코드가 캡슐 충돌을 끄므로 바닥 아래로 떨어지지 않게 이동을 끈다
		Enemy->GetCharacterMovement()->DisableMovement();
		Enemy->GetMesh()->SetSimulatePhysics(false);
		Enemy->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		UAnimInstance* AnimInstance = Enemy->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->StopAllMontages(0.1f);
			if (DeathMontage)
			{
				AnimInstance->Montage_Play(DeathMontage);
			}
		}
	}

	if (UWidgetComponent* HealthBar = Enemy->FindComponentByClass<UWidgetComponent>())
	{
		HealthBar->SetVisibility(false);
	}

	for (const TWeakObjectPtr<AActor>& Summon : SpawnedSummons)
	{
		if (Summon.IsValid())
		{
			Summon->Destroy();
		}
	}
	SpawnedSummons.Reset();

	OnMobDied.Broadcast(Enemy);
	if (USoulMobEncounterSubsystem* Encounter = GetWorld()->GetSubsystem<USoulMobEncounterSubsystem>())
	{
		Encounter->NotifyMobKilled(Enemy, this);
	}
}
