
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "SoulAbilitySystemComponent.generated.h"

class USoulAbilitySystemComponent;
class USoulSkillAbility;

// 에디터에서 슬롯마다 설정할 체인 구조체
USTRUCT(BlueprintType)
struct FSoulSkillSlotChain
{
	GENERATED_BODY()
	
	// 슬롯 입력 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = SkillChain)
	FGameplayTag SlotInputTag;
	// 순서대로 발동할 체인 스킬 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = SkillChain)
	TArray<TSubclassOf<UGameplayAbility>>ChainAbilities;
	// 스킬이 종료된 후 다음 체인 단계가 유지되는 유예시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = SkillChain, meta = (ClampMin = "0.5"))
	float ChainResetDelay = 3.0f;
};

// 런타임에 ASC가 추적하는 슬롯별 체인 상태
struct FSoulSkillChainRuntimeState
{
	FGameplayTag SlotInputTag;
	TArray<FGameplayAbilitySpecHandle> SpecHandles;
	int32 CurrentStep = 0;
	float ChainResetDelay = 3.0f;
	FTimerHandle ResetTimerHandle;
};


UCLASS()
class PROJECTSW_API USoulAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:
	void OnAbilityActorInfoSet();
	
	void GiveStartupAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities);
	
	void GiveSkillSlotChains(const TArray<FSoulSkillSlotChain>& SkillChains);
	
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	
	void AbilityInputTagHeld(const FGameplayTag& InputTag);
	
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
	
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;	
	
public:
	// USoulSkillAbility 종료 시 체인 단계 전진 및 3초 타이머 구동
	void NotifySkillChainAbilityEnded(const FGameplayTag& SlotInputTag, int32 FinishedStep, bool bWasCancelled, bool bTransitioningToNext);
	// 선입력 또는 후딜레이 구간에서 다음 체인 단계로 전환 시도
	bool TryAdvanceChainFromBranch(const FGameplayTag& SlotInputTag);
	
protected:
	UFUNCTION(Client, Reliable)
	void Client_EffectApplied(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle);
	
private:
	bool HandleSkillSlotInput(const FGameplayTag& InputTag);
	// StartStep부터 체인 순서대로 검사하여, 쿨타임 중인 단계(예: 1-1)는 건너뛰고 사용 가능한 단계(예: 1-2) 인덱스를 반환 (없으면 INDEX_NONE)
	int32 FindAvailableChainStep(const FSoulSkillChainRuntimeState& ChainState, int32 StartStep, bool bAllowWrapToStart) const;
	bool TryActivateSlotChainStep(FSoulSkillChainRuntimeState& ChainState, int32 StartStep, bool bAllowWrapToStart, USoulSkillAbility* ActiveSkillToCancel);
	USoulSkillAbility* FindCurrentlyActiveSkillAbility() const;
	void ResetSkillChainSlot(FGameplayTag SlotInputTag);
private:
	TMap<FGameplayTag, FSoulSkillChainRuntimeState> SkillChainStates;
};