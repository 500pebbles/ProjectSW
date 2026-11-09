#include "Run/Combat/SoulCombatFeelSubsystem.h"
#include "Run/Combat/SoulCombatFeelData.h"
#include "Run/Core/SoulRunConfig.h"
#include "Run/Core/SoulRunGameComponent.h"
#include "Run/Core/SoulRunSubsystem.h"
#include "Run/Flow/SoulDungeonFlowData.h"
#include "Run/Mob/SoulMobComponent.h"
#include "GAS/GA/SoulMeleeAbility.h"
#include "Character/SoulCharacterEnemy.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Utility/SoulGameplayTags.h"
#include "Utility/SoulLog.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraShakeBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/RootMotionSource.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

static TAutoConsoleVariable<int32> CVarSoulFeelEnable(TEXT("SW.Feel.Enable"), 1, TEXT("런 타격감 (역경직, 셰이크, FX, 넉백, 포이즈). 0이면 끔"));
static TAutoConsoleVariable<int32> CVarSoulFeelLog(TEXT("SW.Feel.Log"), 0, TEXT("런 타격감 로그 (행 이름, 정지 시간)"));

USoulCombatFeelSubsystem* USoulCombatFeelSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<USoulCombatFeelSubsystem>() : nullptr;
}

TStatId USoulCombatFeelSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USoulCombatFeelSubsystem, STATGROUP_Tickables);
}

const USoulCombatFeelData* USoulCombatFeelSubsystem::GetData() const
{
	if (!bDataResolved)
	{
		const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(GetWorld());
		CachedData = Config ? Config->CombatFeel.Get() : nullptr;
		bDataResolved = Config != nullptr;
	}
	return CachedData;
}

void USoulCombatFeelSubsystem::NotifyDamage(AActor* Source, AActor* Target, float Damage, bool bFatal, bool bCritical, const FGameplayEffectContextHandle& Context)
{
	if (CVarSoulFeelEnable.GetValueOnGameThread() == 0 || !Target)
		return;
	if (USoulCombatFeelSubsystem* Feel = Get(Target))
	{
		Feel->HandleDamage(Source, Target, Damage, bFatal, bCritical, Context);
	}
}

const FSoulHitFeelRow* USoulCombatFeelSubsystem::FindRow(AActor* Source, AActor* Target, float Damage, const FGameplayEffectContextHandle& Context, FName& OutRowName) const
{
	const USoulCombatFeelData* Data = GetData();
	if (!Data || !Data->HitFeelTable)
		return nullptr;

	const APawn* TargetPawn = Cast<APawn>(Target);
	if (TargetPawn && TargetPawn->IsPlayerControlled())
	{
		// 플레이어가 맞음: 최대 체력 비율로 가벼움과 무거움
		const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		const float MaxHealth = ASC ? ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute()) : 0.f;
		OutRowName = MaxHealth > 0.f && Damage >= MaxHealth * Data->PlayerHurtHeavyRatio ? FName(TEXT("PlayerHurtHeavy")) : FName(TEXT("PlayerHurtLight"));
	}
	else
	{
		FString Name = TEXT("Default");
		const UGameplayAbility* Ability = Context.GetAbility();
		if (!Ability)
		{
			// 근접 피해 컨텍스트에는 어빌리티가 안 실려 오므로, 공격자 ASC에서 지금 활성인 근접 어빌리티를 찾는다
			if (const UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Source))
			{
				for (const FGameplayAbilitySpec& Spec : SourceASC->GetActivatableAbilities())
				{
					if (Spec.IsActive() && Spec.Ability && Spec.Ability->IsA<USoulMeleeAbility>())
					{
						Ability = Spec.Ability;
					}
				}
			}
		}
		if (Ability)
		{
			Name = Ability->GetClass()->GetName();
			Name.RemoveFromEnd(TEXT("_C"));
			// 평타는 몽타주 섹션까지 (GA_Attack.Attack3)
			if (const ACharacter* Character = Cast<ACharacter>(Source))
			{
				if (const UAnimInstance* Anim = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
				{
					const FName Section = Anim->Montage_GetCurrentSection();
					const FName WithSection(*FString::Printf(TEXT("%s.%s"), *Name, *Section.ToString()));
					if (!Section.IsNone() && Data->HitFeelTable->FindRow<FSoulHitFeelRow>(WithSection, TEXT(""), false))
					{
						Name = WithSection.ToString();
					}
				}
			}
		}
		OutRowName = FName(*Name);
	}
	if (const FSoulHitFeelRow* Row = Data->HitFeelTable->FindRow<FSoulHitFeelRow>(OutRowName, TEXT(""), false))
		return Row;
	if (!WarnedRows.Contains(OutRowName))
	{
		WarnedRows.Add(OutRowName);
		SW_LOG_WARNING("[FEEL] DT_HitFeel 행 없음 %s → Default (행마다 1회)", *OutRowName.ToString());
	}
	OutRowName = TEXT("Default");
	return Data->HitFeelTable->FindRow<FSoulHitFeelRow>(OutRowName, TEXT(""), false);
}

void USoulCombatFeelSubsystem::HandleDamage(AActor* Source, AActor* Target, float Damage, bool bFatal, bool bCritical, const FGameplayEffectContextHandle& Context)
{
	const USoulCombatFeelData* Data = GetData();
	if (!Data)
		return;
	FName RowName;
	const FSoulHitFeelRow* Row = FindRow(Source, Target, Damage, Context, RowName);
	if (!Row)
		return;

	const double Now = FPlatformTime::Seconds();
	const float CritScale = bCritical ? Data->CritScale : 1.f;
	const FSoulPoiseProfile* Profile = nullptr;
	if (const ASoulCharacterEnemy* Enemy = Cast<ASoulCharacterEnemy>(Target))
	{
		Profile = Data->PoiseProfiles.Find(Enemy->GetEnemyType());
	}
	const float StopScale = Profile ? Profile->HitStopScale : 1.f;

	// 역경직: 공격자는 간격당 한 번, 피격자는 매번
	if (Source && Source != Target && Row->AttackerStop > 0.f && Now - LastAttackerStop >= Data->AttackerStopCooldown)
	{
		LastAttackerStop = Now;
		Freeze(Source, Row->AttackerStop * CritScale);
	}
	if (Row->TargetStop > 0.f && !bFatal)
	{
		Freeze(Target, Row->TargetStop * CritScale * StopScale);
	}

	// 셰이크: 플레이어가 때렸거나 맞았을 때
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (PC && Row->Shake && (Source == PlayerPawn || Target == PlayerPawn) && Now - LastShake >= Data->ShakeCooldown)
	{
		LastShake = Now;
		PC->ClientStartCameraShake(Row->Shake, Row->ShakeScale * CritScale);
	}

	// 임팩트 FX와 소리 (피격자 몸 중앙)
	FVector ImpactPoint = Target->GetActorLocation();
	if (const ACharacter* TargetCharacter = Cast<ACharacter>(Target); TargetCharacter && Source)
	{
		const float Radius = TargetCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
		ImpactPoint += (Source->GetActorLocation() - ImpactPoint).GetSafeNormal2D() * Radius * 0.6f;
	}
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Row->ImpactFX, ImpactPoint);
	if (Row->ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Row->ImpactSound, ImpactPoint, Row->Volume);
	}

	if (Target == PlayerPawn)
	{
		// 플레이어 피격 무적 (죽음이면 안 걺)
		if (!bFatal && Data->PlayerHurtInvincibleSec > 0.f)
		{
			UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
			if (ASC && !InvincibleUntil.Contains(Target))
			{
				ASC->AddLooseGameplayTag(SoulTags::States::Invincible);
			}
			InvincibleUntil.Add(Target, Now + Data->PlayerHurtInvincibleSec);
		}
	}
	else if (!bFatal)
	{
		ApplyPoise(Target, *Row);
		const float KnockScale = Profile ? Profile->KnockbackScale : 1.f;
		ApplyKnockback(Source, Target, Row->KnockbackSpeed * KnockScale, Row->KnockbackTime);
	}

	// 보스 처치 슬로모 (마지막 층 보스는 더 길게)
	if (bFatal)
	{
		const USoulMobComponent* Mob = USoulMobComponent::Find(Target);
		if (Mob && Mob->GetRole() == ESoulMobRole::Boss)
		{
			const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(this);
			const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
			const bool bFinal = Config && Config->DungeonFlow && Run && Run->GetState().Floor >= Config->DungeonFlow->Floors.Num();
			const FSoulSlomo& Slomo = bFinal ? Data->FinalBossKillSlomo : Data->BossKillSlomo;
			StartSlomo(Slomo.Dilation, Slomo.Duration);
		}
	}

	if (CVarSoulFeelLog.GetValueOnGameThread() > 0)
	{
		SW_LOG("[FEEL] %s %s -> %s dmg=%.0f crit=%d fatal=%d stop=%.3f/%.3f", *RowName.ToString(), Source ? *Source->GetName() : TEXT("none"), *Target->GetName(), Damage, bCritical, bFatal,
			Row->AttackerStop * CritScale, Row->TargetStop * CritScale * StopScale);
	}
	OnHitConfirmed.Broadcast(Source, Target, Damage, RowName);
}

void USoulCombatFeelSubsystem::Freeze(AActor* Actor, float Seconds)
{
	const USoulCombatFeelData* Data = GetData();
	if (!Actor || !Data || Seconds <= 0.f)
		return;
	Actor->CustomTimeDilation = Data->FreezeDilation;
	double& Until = FrozenUntil.FindOrAdd(Actor);
	Until = FMath::Max(Until, FPlatformTime::Seconds() + Seconds);
}

void USoulCombatFeelSubsystem::ApplyPoise(AActor* Target, const FSoulHitFeelRow& Row)
{
	const USoulCombatFeelData* Data = GetData();
	const ASoulCharacterEnemy* Enemy = Cast<ASoulCharacterEnemy>(Target);
	const FSoulPoiseProfile* Profile = Data && Enemy ? Data->PoiseProfiles.Find(Enemy->GetEnemyType()) : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!Profile || !ASC || (!Profile->bNeverStagger && Profile->MaxPoise <= 0.f))
		return;

	const FGameplayTagContainer HitReact(SoulTags::Combats::HitReact);
	FPoiseState& State = Poise.FindOrAdd(Target);
	const double Now = FPlatformTime::Seconds();
	State.LastHit = Now;
	bool bStagger = false;
	if (!Profile->bNeverStagger && Now >= State.CooldownUntil)
	{
		State.Value += Row.Poise;
		if (State.Value >= Profile->MaxPoise)
		{
			bStagger = true;
			State.Value = 0.f;
			State.CooldownUntil = Now + Profile->StaggerCooldown;
		}
	}
	// 평소에는 경직을 막고, 포이즈를 넘긴 그 타격만 연다 (다음 틱에 다시 막음)
	if (bStagger && State.bBlocked)
	{
		ASC->UnBlockAbilitiesWithTags(HitReact);
		State.bBlocked = false;
		State.bReblockNextTick = true;
	}
	else if (!bStagger && !State.bBlocked)
	{
		ASC->BlockAbilitiesWithTags(HitReact);
		State.bBlocked = true;
	}
	if (bStagger)
	{
		State.bReblockNextTick = true;
	}
	if (CVarSoulFeelLog.GetValueOnGameThread() > 0)
	{
		SW_LOG("[FEEL] poise %s value=%.0f/%.0f stagger=%d", *Target->GetName(), State.Value, Profile->MaxPoise, bStagger);
	}
}

void USoulCombatFeelSubsystem::ApplyKnockback(AActor* Source, AActor* Target, float Speed, float Time)
{
	ACharacter* Character = Cast<ACharacter>(Target);
	if (!Character || !Source || Speed <= 0.f || Time <= 0.f)
		return;
	const FVector Direction = (Target->GetActorLocation() - Source->GetActorLocation()).GetSafeNormal2D();
	TSharedPtr<FRootMotionSource_ConstantForce> Force = MakeShared<FRootMotionSource_ConstantForce>();
	Force->InstanceName = TEXT("SoulKnockback");
	Force->AccumulateMode = ERootMotionAccumulateMode::Additive;
	Force->Priority = 5;
	Force->Force = Direction * Speed;
	Force->Duration = Time;
	Force->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	Force->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
	Character->GetCharacterMovement()->ApplyRootMotionSource(Force);
}

void USoulCombatFeelSubsystem::StartSlomo(float Dilation, float Duration)
{
	if (Duration <= 0.f || Dilation >= 1.f)
		return;
	UGameplayStatics::SetGlobalTimeDilation(this, Dilation);
	SlomoUntil = FPlatformTime::Seconds() + Duration;
}

void USoulCombatFeelSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	for (auto It = FrozenUntil.CreateIterator(); It; ++It)
	{
		if (!It->Key.IsValid())
		{
			It.RemoveCurrent();
		}
		else if (Now >= It->Value)
		{
			It->Key->CustomTimeDilation = 1.f;
			It.RemoveCurrent();
		}
	}
	for (auto It = InvincibleUntil.CreateIterator(); It; ++It)
	{
		if (!It->Key.IsValid())
		{
			It.RemoveCurrent();
		}
		else if (Now >= It->Value)
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(It->Key.Get()))
			{
				ASC->RemoveLooseGameplayTag(SoulTags::States::Invincible);
			}
			It.RemoveCurrent();
		}
	}
	if (SlomoUntil > 0.0 && Now >= SlomoUntil)
	{
		SlomoUntil = 0.0;
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	}
	const USoulCombatFeelData* Data = GetData();
	for (auto It = Poise.CreateIterator(); It; ++It)
	{
		AActor* Actor = It->Key.Get();
		if (!Actor)
		{
			It.RemoveCurrent();
			continue;
		}
		FPoiseState& State = It->Value;
		const ASoulCharacterEnemy* Enemy = Cast<ASoulCharacterEnemy>(Actor);
		const FSoulPoiseProfile* Profile = Data && Enemy ? Data->PoiseProfiles.Find(Enemy->GetEnemyType()) : nullptr;
		if (State.bReblockNextTick)
		{
			State.bReblockNextTick = false;
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor); ASC && !State.bBlocked)
			{
				ASC->BlockAbilitiesWithTags(FGameplayTagContainer(SoulTags::Combats::HitReact));
				State.bBlocked = true;
			}
		}
		if (Profile && State.Value > 0.f && Now - State.LastHit >= Profile->RegenDelay)
		{
			State.Value = FMath::Max(0.f, State.Value - Profile->RegenPerSec * DeltaTime);
		}
	}
}
