#include "SoulSkillAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "GAS/Core/SoulAbilitySystemComponent.h"
#include "Interface/SoulCombatInterface.h"
#include "Player/SoulPlayerController.h"
#include "Utility/SoulGameplayTags.h"
#include "Utility/SoulLog.h"

USoulSkillAbility::USoulSkillAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 어빌리티 태그: Skill
	FGameplayTagContainer SkillAssetTags;
	SkillAssetTags.AddTag(SoulTags::Abilities::Skill);
	SetAssetTags(SkillAssetTags);

	// 평타(Attack) 도중 스킬을 누르면 즉시 평타를 캔슬하고 발동
	CancelAbilitiesWithTag.AddTag(SoulTags::Abilities::Attack);
	BlockAbilitiesWithTag.AddTag(SoulTags::Abilities::Attack);

	// 실행 중 States.Attacking 부여 → 이동 잠금 및 방향 입력 기록 연동
	ActivationOwnedTags.AddTag(SoulTags::States::Attacking);

	// 사망 또는 대시 중에는 발동 불가
	ActivationBlockedTags.AddTag(SoulTags::States::Dead);
	ActivationBlockedTags.AddTag(SoulTags::States::Dashing);
}

void USoulSkillAbility::SetChainRuntimeInfo(const FGameplayTag& InSlotTag, int32 InChainStep, bool bInIsChainFinisher)
{
	AssignedSlotTag = InSlotTag;
	AssignedChainStep = InChainStep;
	bIsChainFinisher = bInIsChainFinisher;
}

bool USoulSkillAbility::TryBufferOrBranchNextChain()
{
	// 이미 Branch 시점을 지났다면(후딜레이 구간) 즉시 다음 체인으로 전환 허용
	if (bPassedBranchPoint)
	{
		return true;
	}

	// 선입력 구간(ANS_ComboWindow) 안이라면 예약만 걸어두고 Branch 시점에 전환
	if (bCancelWindowOpen)
	{
		bNextChainBuffered = true;
		SW_LOG("[Skill] Chain input buffered on %s (Slot: %s, Step: %d)", *GetName(), *AssignedSlotTag.ToString(), AssignedChainStep);
	}

	return false;
}

void USoulSkillAbility::FinishForChainTransition()
{
	bTransitioningToNextChain = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

const FGameplayTagContainer* USoulSkillAbility::GetCooldownTags() const
{
	TempCooldownTags.Reset();
	if (const FGameplayTagContainer* ParentTags = Super::GetCooldownTags())
	{
		TempCooldownTags.AppendTags(*ParentTags);
	}
	if (CooldownTag.IsValid())
	{
		TempCooldownTags.AddTag(CooldownTag);
	}
	return &TempCooldownTags;
}

void USoulSkillAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	const float BaseDuration = CooldownDuration.GetValueAtLevel(GetAbilityLevel());
	if (!CooldownGE || !CooldownTag.IsValid() || BaseDuration <= 0.0f) return;

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownGameplayEffectClass, GetAbilityLevel());
	if (!SpecHandle.IsValid()) return;

	const float FinalDuration = bIsChainFinisher ? (BaseDuration * ChainFinisherCooldownMultiplier) : BaseDuration;
	SpecHandle.Data->DynamicGrantedTags.AddTag(CooldownTag);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, SoulTags::Cooldown::Duration, FinalDuration);

	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
}

void USoulSkillAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!SkillMontage)
	{
		SW_LOG_WARNING("[Skill] %s has no SkillMontage assigned!", *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 상태 초기화
	HitActors.Empty();
	bCancelWindowOpen = false;
	bPassedBranchPoint = false;
	bNextChainBuffered = false;
	bTransitioningToNextChain = false;

	// 입력 방향으로 회전
	RotateToInputDirection();
	

	SW_LOG("[Skill] Activated %s (Slot: %s, Step: %d, Finisher: %d)", *GetName(), *AssignedSlotTag.ToString(), AssignedChainStep, bIsChainFinisher ? 1 : 0);

	// 1. 몽타주 재생 태스크
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SkillMontage);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	// 2. 칼날 판정 이벤트 수신 (TraceBegin / Hit)
	UAbilityTask_WaitGameplayEvent* TraceBeginTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SoulTags::Events::Melee::TraceBegin);
	TraceBeginTask->EventReceived.AddDynamic(this, &ThisClass::OnTraceBeginEvent);
	TraceBeginTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SoulTags::Events::Melee::Hit);
	HitTask->EventReceived.AddDynamic(this, &ThisClass::OnMeleeHitEvent);
	HitTask->ReadyForActivation();

	// 3. 체인 선입력 및 전환 시점 이벤트 수신 (기존 ANS_ComboWindow / AN_ComboBranch 재사용)
	UAbilityTask_WaitGameplayEvent* ComboOpenTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SoulTags::Events::Combo::Open);
	ComboOpenTask->EventReceived.AddDynamic(this, &ThisClass::OnComboOpenEvent);
	ComboOpenTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* ComboCloseTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SoulTags::Events::Combo::Close);
	ComboCloseTask->EventReceived.AddDynamic(this, &ThisClass::OnComboCloseEvent);
	ComboCloseTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* ComboBranchTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SoulTags::Events::Combo::Branch);
	ComboBranchTask->EventReceived.AddDynamic(this, &ThisClass::OnComboBranchEvent);
	ComboBranchTask->ReadyForActivation();
}

void USoulSkillAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	HitActors.Empty();
	bCancelWindowOpen = false;
	bPassedBranchPoint = false;
	bNextChainBuffered = false;

	// ASC에 스킬 종료 알림 → 다음 체인 단계(1-1, 1-2) 전진 및 3초 리셋 타이머 시작
	if (USoulAbilitySystemComponent* SoulASC = Cast<USoulAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		if (AssignedSlotTag.IsValid())
		{
			SoulASC->NotifySkillChainAbilityEnded(AssignedSlotTag, AssignedChainStep, bWasCancelled, bTransitioningToNextChain);
		}
	}

	bTransitioningToNextChain = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USoulSkillAbility::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USoulSkillAbility::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USoulSkillAbility::OnTraceBeginEvent(FGameplayEventData Payload)
{
	// 새 칼날 휘두르기 구간이 시작될 때마다 피격 목록 초기화 (다단히트 스킬 지원)
	HitActors.Empty();
}

void USoulSkillAbility::OnMeleeHitEvent(FGameplayEventData Payload)
{
	AActor* TargetActor = const_cast<AActor*>(Payload.Target.Get());
	if (!IsValid(TargetActor) || HitActors.Contains(TargetActor)) return;

	// 죽은 대상 무시
	if (TargetActor->Implements<USoulCombatInterface>() && ISoulCombatInterface::Execute_IsDead(TargetActor))
	{
		return;
	}

	HitActors.Add(TargetActor);

	const float FinalMultiplier = bIsChainFinisher ? (DamageMultiplier * ChainFinisherDamageBonus) : DamageMultiplier;
	ApplyMeleeDamage(TargetActor, FinalMultiplier);
}

void USoulSkillAbility::OnComboOpenEvent(FGameplayEventData Payload)
{
	bCancelWindowOpen = true;
}

void USoulSkillAbility::OnComboCloseEvent(FGameplayEventData Payload)
{
	bCancelWindowOpen = false;
}

void USoulSkillAbility::OnComboBranchEvent(FGameplayEventData Payload)
{
	bPassedBranchPoint = true;

	// 선입력 구간에 같은 슬롯 버튼을 미리 눌러두었다면 Branch 시점에 즉시 다음 체인 스킬 발동
	if (bNextChainBuffered)
	{
		bNextChainBuffered = false;
		if (USoulAbilitySystemComponent* SoulASC = Cast<USoulAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
		{
			SoulASC->TryAdvanceChainFromBranch(AssignedSlotTag);
		}
	}
}

void USoulSkillAbility::RotateToInputDirection()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character) return;

	ASoulPlayerController* SoulPC = Cast<ASoulPlayerController>(Character->GetController());
	if (!SoulPC) return;

	FVector InputDir = SoulPC->GetMoveInputDirection();
	if (InputDir.IsNearlyZero())
	{
		InputDir = SoulPC->ConsumeAttackInputDirection();
	}
	else
	{
		SoulPC->ConsumeAttackInputDirection();
	}

	if (!InputDir.IsNearlyZero())
	{
		const FRotator NewRotation(0.0f, InputDir.Rotation().Yaw, 0.0f);
		Character->SetActorRotation(NewRotation);
	}
}