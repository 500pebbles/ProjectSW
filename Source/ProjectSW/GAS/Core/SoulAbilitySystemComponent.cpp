
#include "SoulAbilitySystemComponent.h"

#include "Engine/World.h"
#include "GAS/GA/SoulGameplayAbility.h"
#include "GAS/GA/SoulSkillAbility.h"
#include "TimerManager.h"
#include "Utility/SoulGameplayTags.h"
#include "Utility/SoulLog.h"


void USoulAbilitySystemComponent::OnAbilityActorInfoSet()
{
	/* InitAbilityActorInfo로 인해 ASC가 등록되었을 경우 호출되는 Hook. 
	 * 모든 종류의 GameplayEffect 적용에 반응하는 콜백함수를 바인딩. 
	 * 현재 용도 : Widget에 바인딩하고 Effect적용시마다 UI Call 
	 * 최종적인 호출위치가 PossessedBy, 바인딩 자체는 서버가 보유한 ASC만 이루어짐. */
	
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &ThisClass::Client_EffectApplied);	
}

void USoulAbilitySystemComponent::GiveStartupAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
	/* Startup Ability는 기본적으로  Input GameplayTag와 함께 부여. */
	
	for (const auto AbilityClass : StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		if (const USoulGameplayAbility* SoulAbility = Cast<USoulGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.DynamicAbilityTags.AddTag(SoulAbility->StartupInputTag);			
			GiveAbility(AbilitySpec);
		}		
	}
}

void USoulAbilitySystemComponent::GiveSkillSlotChains(const TArray<FSoulSkillSlotChain>& SkillChains)
{ 
	SkillChainStates.Empty();
	
	for (const FSoulSkillSlotChain& ChainConfig : SkillChains)
	{
		if (!ChainConfig.SlotInputTag.IsValid() || ChainConfig.ChainAbilities.IsEmpty()) continue;
		
		FSoulSkillChainRuntimeState RuntimeState;
		RuntimeState.SlotInputTag = ChainConfig.SlotInputTag;
		RuntimeState.ChainResetDelay = ChainConfig.ChainResetDelay;
		RuntimeState.CurrentStep = 0;
		for (const TSubclassOf<UGameplayAbility>& AbilityClass : ChainConfig.ChainAbilities)
		{
			if (!AbilityClass) continue;
			FGameplayAbilitySpec Spec(AbilityClass, 1);
			const FGameplayAbilitySpecHandle Handle = GiveAbility(Spec);
			RuntimeState.SpecHandles.Add(Handle);
		}
		if (!RuntimeState.SpecHandles.IsEmpty())
		{
			SkillChainStates.Add(ChainConfig.SlotInputTag, MoveTemp(RuntimeState));
		}
	}
}

void USoulAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	/* 입력 정책 : 누른 순간 1회.
	 * 비활성 어빌리티는 발동, 실행 중인 어빌리티에는 InputPressed 이벤트만 전달 (WaitInputPress로 콤보 입력 수신). */
	if (!InputTag.IsValid()) return;
	if (HandleSkillSlotInput(InputTag))
	{
		return;
	}
	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag))
		{
			AbilitySpecInputPressed(AbilitySpec);  // Set flag + 실행 중이면 InputPressed 이벤트.
			if (!AbilitySpec.IsActive()) TryActivateAbility(AbilitySpec.Handle); // Activate Ability.
		}
	}
}

void USoulAbilitySystemComponent::AbilityInputTagHeld(const FGameplayTag& InputTag)
{
	/* 입력 정책 변경으로 Held에서는 발동·입력 전달을 하지 않음. (매 프레임 호출되어 콤보 입력으로 오인되는 문제)
	* 차지형처럼 누르고 있는 동안 동작하는 어빌리티가 생기면 여기서 처리. */
}

void USoulAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;	
	
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag))
		{
			AbilitySpecInputReleased(AbilitySpec);  // Just set flag.
		}
	}
}

void USoulAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);
	
	/* 엔진 기본 구현은 InputPressed 플래그만 세우고 이벤트를 보내지 않아 WaitInputPress가 동작하지 않음.
	 * 실행 중인 어빌리티에 InputPressed 이벤트를 보내 WaitInputPress로 받을 수 있게 함. (Lyra와 같은 방식) */
	if (Spec.IsActive())
	{
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		const FPredictionKey PredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : FPredictionKey();
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, PredictionKey);
	}
}

void USoulAbilitySystemComponent::NotifySkillChainAbilityEnded(const FGameplayTag& SlotInputTag, int32 FinishedStep,
	bool bWasCancelled, bool bTransitioningToNext)
{
	// 이미 TryAdvanceChainFromBranch를 통해 다음 체인으로 즉시 넘어가는 중이면 타이머/단계 처리를 생략
	if (bTransitioningToNext) return;
	FSoulSkillChainRuntimeState* ChainState = SkillChainStates.Find(SlotInputTag);
	if (!ChainState || ChainState->SpecHandles.IsEmpty()) return;
	const int32 NumSteps = ChainState->SpecHandles.Num();
	const int32 NextStep = FinishedStep + 1;
	// 마지막 단계(1-2)까지 다 썼다면 즉시 1단계(0)로 리셋
	if (NextStep >= NumSteps)
	{
		ChainState->CurrentStep = 0;
		SW_LOG("[SkillChain] Slot %s finished final step (%d) -> Reset to Step 0", *SlotInputTag.ToString(), FinishedStep);
		return;
	}
	// 아직 다음 단계(1-1 또는 1-2)가 남아있다면 다음 단계로 올려두고 3초 유예 타이머 시작
	ChainState->CurrentStep = NextStep;
	if (UWorld* World = GetWorld())
	{
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(this, &ThisClass::ResetSkillChainSlot, SlotInputTag);
		World->GetTimerManager().SetTimer(ChainState->ResetTimerHandle, TimerDelegate, ChainState->ChainResetDelay, false);
	}
	SW_LOG("[SkillChain] Slot %s advanced to Step %d (Reset in %.1fs)", *SlotInputTag.ToString(), NextStep, ChainState->ChainResetDelay);
}

bool USoulAbilitySystemComponent::TryAdvanceChainFromBranch(const FGameplayTag& SlotInputTag)
{
	FSoulSkillChainRuntimeState* ChainState = SkillChainStates.Find(SlotInputTag);
	if (!ChainState) return false;
	USoulSkillAbility* ActiveSkill = FindCurrentlyActiveSkillAbility();
	if (!ActiveSkill || !ActiveSkill->GetAssignedSlotTag().MatchesTagExact(SlotInputTag)) return false;
	const int32 NumSteps = ChainState->SpecHandles.Num();
	const int32 NextStep = ActiveSkill->GetAssignedChainStep() + 1;
	// 이미 마지막 단계(예: 1-2)를 실행 중이었다면 더 이어갈 뒷단계가 없으므로 불발
	if (NextStep >= NumSteps)
	{
		return false;
	}
	// 소울워커 원작 방식: NextStep(예: 1-1)부터 뒷방향으로만 탐색하여 쿨타임이면 1-2로 자동 스킵, 뒷단계가 전부 쿨타임이면 불발(bAllowWrapToStart = false)
	return TryActivateSlotChainStep(*ChainState, NextStep, false, ActiveSkill);
}
int32 USoulAbilitySystemComponent::FindAvailableChainStep(const FSoulSkillChainRuntimeState& ChainState, int32 StartStep, bool bAllowWrapToStart) const
{
	const int32 NumSteps = ChainState.SpecHandles.Num();
	if (NumSteps == 0) return INDEX_NONE;
	const int32 ClampedStart = FMath::Clamp(StartStep, 0, NumSteps - 1);
	auto IsStepReady = [this, &ChainState](int32 StepIndex) -> bool
	{
		const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(ChainState.SpecHandles[StepIndex]);
		if (!Spec || !Spec->Ability) return false;
		const UGameplayAbility* InstancedOrCDO = Spec->GetPrimaryInstance() ? Spec->GetPrimaryInstance() : Spec->Ability.Get();
		return InstancedOrCDO->CheckCooldown(Spec->Handle, AbilityActorInfo.Get()) &&
		       InstancedOrCDO->CheckCost(Spec->Handle, AbilityActorInfo.Get());
	};
	// 1차: 현재 체인 단계부터 마지막 단계(예: 1 -> 1-1 -> 1-2) 순서로 검사 (쿨타임 중인 단계는 자동 스킵)
	for (int32 Step = ClampedStart; Step < NumSteps; ++Step)
	{
		if (IsStepReady(Step))
		{
			if (Step != ClampedStart)
			{
				SW_LOG("[SkillChain] Slot %s: Step %d is on cooldown -> Auto-skipped to Step %d", *ChainState.SlotInputTag.ToString(), ClampedStart, Step);
			}
			return Step;
		}
	}
	// 2차: 대기/유예 상태에서 뒷단계(1-1, 1-2)가 모두 쿨타임일 때, 앞단계(1) 쿨타임이 돌아왔다면 1단계부터 발동 허용
	if (bAllowWrapToStart)
	{
		for (int32 Step = 0; Step < ClampedStart; ++Step)
		{
			if (IsStepReady(Step))
			{
				SW_LOG("[SkillChain] Slot %s: Remaining steps on cooldown -> Restarted at available Step %d", *ChainState.SlotInputTag.ToString(), Step);
				return Step;
			}
		}
	}
	return INDEX_NONE;
}

bool USoulAbilitySystemComponent::HandleSkillSlotInput(const FGameplayTag& InputTag)
{
	FSoulSkillChainRuntimeState* ChainState = SkillChainStates.Find(InputTag);
	if (!ChainState || ChainState->SpecHandles.IsEmpty()) return false;
	USoulSkillAbility* ActiveSkill = FindCurrentlyActiveSkillAbility();
	// Case 1: 현재 어떤 스킬이 실행 중일 때
	if (ActiveSkill)
	{
		// 1-A: 같은 슬롯 키를 누른 경우 (예: 1번 스킬 모션 중 다시 1 입력 → 1-1 선입력 또는 후딜 캔슬)
		if (ActiveSkill->GetAssignedSlotTag().MatchesTagExact(InputTag))
		{
			if (ActiveSkill->TryBufferOrBranchNextChain())
			{
				// 이미 AN_ComboBranch를 지난 상태(후딜레이)라면 즉시 다음 체인으로 전환 시도
				TryAdvanceChainFromBranch(InputTag);
			}
			return true;
		}
		// 1-B: 다른 슬롯 키를 누른 경우 (예: 1번 스킬 중 2번 입력)
		// 현재 스킬이 AN_ComboBranch(후딜레이 캔슬 시점)를 지났을 때만 다른 스킬로 캔슬 허용
		if (!ActiveSkill->CanBeCanceledBySkill())
		{
			return true;
		}
		TryActivateSlotChainStep(*ChainState, ChainState->CurrentStep, true, ActiveSkill);
		return true;
	}
	// Case 2: 실행 중인 스킬이 없을 때 (대기 상태, 평타 중, 또는 이전 스킬 종료 후 3초 체인 유예 중)
	TryActivateSlotChainStep(*ChainState, ChainState->CurrentStep, true, nullptr);
	return true;
}

bool USoulAbilitySystemComponent::TryActivateSlotChainStep(FSoulSkillChainRuntimeState& ChainState, int32 StartStep,
                                                           bool bAllowWrapToStart, USoulSkillAbility* ActiveSkillToCancel)
{
	// 사망/대시 등 발동 불가 상태면 기존 스킬을 캔슬하지 않고 즉시 중단
	if (HasMatchingGameplayTag(SoulTags::States::Dead) || HasMatchingGameplayTag(SoulTags::States::Dashing))
	{
		return false;
	}
	const int32 SelectedStep = FindAvailableChainStep(ChainState, StartStep, bAllowWrapToStart);
	if (SelectedStep == INDEX_NONE)
	{
		SW_LOG("[SkillChain] No available skill in slot %s (all candidate steps on cooldown).", *ChainState.SlotInputTag.ToString());
		return false;
	}
	FGameplayAbilitySpec* SelectedSpec = FindAbilitySpecFromHandle(ChainState.SpecHandles[SelectedStep]);
	if (!SelectedSpec) return false;
	// 이전 스킬 후딜레이를 끊고 넘어가는 경우 먼저 이전 스킬을 깔끔하게 종료
	if (ActiveSkillToCancel)
	{
		if (ActiveSkillToCancel->GetAssignedSlotTag().MatchesTagExact(ChainState.SlotInputTag))
		{
			ActiveSkillToCancel->FinishForChainTransition();
		}
		else
		{
			CancelAbilityHandle(ActiveSkillToCancel->GetCurrentAbilitySpecHandle());
		}
	}
	// 3초 리셋 타이머가 돌고 있었다면 해제
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChainState.ResetTimerHandle);
	}
	const int32 NumSteps = ChainState.SpecHandles.Num();
	ChainState.CurrentStep = SelectedStep;
	const bool bIsFinisher = (NumSteps >= 2) && (SelectedStep == NumSteps - 1);
	if (USoulSkillAbility* SkillInstance = Cast<USoulSkillAbility>(SelectedSpec->GetPrimaryInstance()))
	{
		SkillInstance->SetChainRuntimeInfo(ChainState.SlotInputTag, SelectedStep, bIsFinisher);
	}
	else if (USoulSkillAbility* SkillCDO = Cast<USoulSkillAbility>(SelectedSpec->Ability))
	{
		SkillCDO->SetChainRuntimeInfo(ChainState.SlotInputTag, SelectedStep, bIsFinisher);
	}
	return TryActivateAbility(SelectedSpec->Handle);
}

USoulSkillAbility* USoulAbilitySystemComponent::FindCurrentlyActiveSkillAbility() const
{
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.IsActive())
		{
			if (USoulSkillAbility* SkillAbility = Cast<USoulSkillAbility>(Spec.GetPrimaryInstance()))
			{
				return SkillAbility;
			}
		}
	}
	return nullptr;
}

void USoulAbilitySystemComponent::ResetSkillChainSlot(FGameplayTag SlotInputTag)
{
	if (FSoulSkillChainRuntimeState* ChainState = SkillChainStates.Find(SlotInputTag))
	{
		ChainState->CurrentStep = 0;
		SW_LOG("[SkillChain] Slot %s chain timer expired -> Reset to Step 0", *SlotInputTag.ToString());
	}
}

void USoulAbilitySystemComponent::Client_EffectApplied_Implementation(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	/* 종류를 불문하고 모든 GameplayEffect의 발생에 실행되는 콜백 함수. 
	 * 콜백의 파라미터로 GE와 ASC에 해당하는 대부분의 데이터에 접근.
	 * GameplayEffect -> ASC 적용은 서버에서만 이루어지고, 콜백 바인딩 자체도 서버에서만 진행 (PossessedBy).
	 * 따라서 Client RPC */
	
	// GameplayEffect가 보유한 태그를 전부 순회하고 원하는 작업을 수행. 현재는 보류상태. 
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);
	for (const FGameplayTag& Tag : TagContainer)
	{
		
	}
}
