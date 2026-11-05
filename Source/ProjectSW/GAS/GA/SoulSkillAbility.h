#pragma once

#include "CoreMinimal.h"
#include "GAS/GA/SoulMeleeAbility.h"
#include "SoulSkillAbility.generated.h"

class UAnimMontage;
class UTexture2D;

UCLASS()
class PROJECTSW_API USoulSkillAbility : public USoulMeleeAbility
{
	GENERATED_BODY()

public:
	USoulSkillAbility();

	// 현재 스킬이 후딜레이 캔슬 허용 시점(AN_ComboBranch)을 지났는지 여부
	bool CanBeCanceledBySkill() const { return bPassedBranchPoint; }

	// 같은 슬롯 키가 다시 눌렸을 때: Branch 이후면 즉시 전환(true 반환), Open 구간이면 선입력 예약(false 반환)
	bool TryBufferOrBranchNextChain();

	// 다음 체인 스킬로 넘어가기 위해 현재 스킬을 정상 종료시킬 때 호출
	void FinishForChainTransition();

	// 슬롯 체인 매니저가 발동 직전에 슬롯 정보와 3단계(Finisher) 여부를 주입
	void SetChainRuntimeInfo(const FGameplayTag& InSlotTag, int32 InChainStep, bool bInIsChainFinisher);

	FGameplayTag GetAssignedSlotTag() const { return AssignedSlotTag; }
	int32 GetAssignedChainStep() const { return AssignedChainStep; }
	
	UTexture2D* GetSkillIcon() const { return SkillIcon; }
	FGameplayTag GetSkillCooldownTag() const { return CooldownTag; }                                                                                                                                                                    

	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnTraceBeginEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMeleeHitEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnComboOpenEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnComboCloseEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnComboBranchEvent(FGameplayEventData Payload);

	void RotateToInputDirection();

protected:
	// 재생할 스킬 몽타주 (단타 또는 다단히트 클립)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UAnimMontage> SkillMontage;
	
	// UI 슬롯에 표시할 스킬 아이콘 텍스처
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|UI")
	TObjectPtr<UTexture2D> SkillIcon;

	// 이 스킬의 기본 데미지 배율 (DamageTypes 기본값에 곱해짐)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill", meta = (ClampMin = "0.1"))
	float DamageMultiplier = 1.5f;

	// 3단계(1-2, 2-2, 3-2) 체인 피니시로 발동될 때 곱해지는 추가 데미지 보너스 배율 (예: 1.25 = 25% 증가)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|ChainBonus", meta = (ClampMin = "1.0"))
	float ChainFinisherDamageBonus = 1.25f;

	// 3단계 체인 피니시로 발동될 때 쿨타임 배율 (예: 0.8 = 쿨타임 20% 감소)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|ChainBonus", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float ChainFinisherCooldownMultiplier = 0.8f;

	// 이 스킬 고유의 쿨타임 태그 (예: SoulTags.Cooldown.Skill.1_0)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Cooldown")
	FGameplayTag CooldownTag;

	// 이 스킬의 쿨타임 시간(초). 0 이하면 쿨타임 없음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Cooldown")
	FScalableFloat CooldownDuration = 5.0f;

private:
	// GetCooldownTags()가 반환할 임시 컨테이너
	UPROPERTY(Transient)
	mutable FGameplayTagContainer TempCooldownTags;

	// 한 번의 MeleeTrace 스윙 구간 동안 중복 타격 방지 목록
	UPROPERTY(Transient)
	TSet<TObjectPtr<AActor>> HitActors;

	FGameplayTag AssignedSlotTag;
	int32 AssignedChainStep = 0;
	bool bIsChainFinisher = false;

	bool bCancelWindowOpen = false;
	bool bPassedBranchPoint = false;
	bool bNextChainBuffered = false;
	bool bTransitioningToNextChain = false;
};