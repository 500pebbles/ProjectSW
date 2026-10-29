
#include "SoulAbilitySystemComponent.h"

#include "GAS/GA/SoulGameplayAbility.h"
#include "Utility/SoulGameplayTags.h"


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

void USoulAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	/* 입력 정책 : 누른 순간 1회.
	 * 비활성 어빌리티는 발동, 실행 중인 어빌리티에는 InputPressed 이벤트만 전달 (WaitInputPress로 콤보 입력 수신). */
	if (!InputTag.IsValid()) return;
	
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