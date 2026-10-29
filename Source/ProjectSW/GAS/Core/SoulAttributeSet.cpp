// 


#include "SoulAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "GAS/Utility/SoulAbilitySystemLibrary.h"
#include "Interface/SoulCombatInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/SoulPlayerController.h"
#include "Utility/SoulGameplayTags.h"

USoulAttributeSet::USoulAttributeSet()
{
	
}

void USoulAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);	
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, HealthRegeneration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, StaminaRegeneration, COND_None, REPNOTIFY_Always);	
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, Defense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, CriticalRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, CriticalDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, CooldownReduction, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, DamageAmplification, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, PhysicalResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, SinResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, VirtueResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, GraceResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, HateResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, TormentResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USoulAttributeSet, SolaceResistance, COND_None, REPNOTIFY_Always);
}

void USoulAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	/* 이중 클램핑.
	 * 전제 : CurrentValue = BaseValue + Modifier.
	 * 10초동안 체력이 반토막났다가 다시 되돌리는 has Duration이벤트 : BaseValue + Modifier = CurrentValue.
	 * 10초동안 매초 체력이 1씩 깎이는 has Duration이벤트 (Period) : BaseValue에 직접 -1. BaseValue = CurrentValue.
	 * PreAttributeChange는 CurrentValue만 클램핑이 가능. 정확히는 변경될 값인 CurrentValue인 "NewValue"만 클램핑.
	 * 문제는 Modifier는 NewValue와 별개라 남아있음. Max(100) 일 때 Base(90) + Modifier(40)  = Current(100)으로 클램핑.
	 * 하지만 남은 기간에 Max(150)으로 늘어날 경우 남은 Modifier가 그대로 유지되어 Current(130)으로 취급되는 문제가 발생.
	 * 따라서 이곳의 클램핑은 Modifier의 초과분을 영구적으로 제거하는 클램핑이 아님. 
	 * 또한 최종적인 CurrentValue 값만 클램핑이 가능할 뿐, BaseValue가 최대치를 넘어서는 상황은 처리하지 못함.
	 * Max(100), Base(90) + Instant(90)일 경우 CurrentValue(100)으로 클램핑은 가능하지만, Base 자체는 Base(180). */	
	
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}	
}

void USoulAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	/* 이중 클램핑.
	 * PostGameplayEffectExecute는 CurrentValue를 클램핑하는 것이 아닌 BaseValue를 직접 "수정".
	 * 또한 Has Duration/Infinite 이펙트에는 호출되지 않음. BaseValue를 수정하는 Instant 이펙트에만 호출.
	 * Max(100), Base(90), Current(90)의 상황에서 Instant GE로 +Base(30)의 상황
	 * 이전 클램핑이 아닌, 실제로 Attribute에 대한 Setter를 호출하고 최대값을 클램핑하는 것.
	 * 결과적으로 PreAttribute는 CurrentValue, Duration에 대한 클램핑
	 * PosetGameplay는 BaseValue, Instant에 대한 후처리 */
	
	// Props 구조체에 GameplayEffect 결과 정보 주입. 
	FEffectProperties Props;
	SetEffectProperties(Data, OUT Props);
	
	// Instant Effect 클램핑, Base Value.
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.f, GetMaxStamina()));
	}
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalIncomingDamage = GetIncomingDamage();
		SetIncomingDamage(0.f);
		
		if (LocalIncomingDamage > 0.f)
		{
			const float NewHealth = GetHealth() - LocalIncomingDamage;
			SetHealth(FMath::Clamp(NewHealth, 0.f, GetMaxHealth()));
			
			const bool bFatal = NewHealth <= 0.f;
			if (bFatal)
			{				
				ISoulCombatInterface* CombatInterface = Cast<ISoulCombatInterface>(Props.TargetAvatarActor);
				if (CombatInterface) CombatInterface->Die();
			}
			else
			{
				FGameplayTagContainer TagContainer; // Temp Tag.
				TagContainer.AddTag(SoulTags::Combats::HitReact);
				Props.TargetASC->TryActivateAbilitiesByTag(TagContainer); 
			}
			
			const bool bIsCriticalHit = USoulAbilitySystemLibrary::IsCriticalHit(Props.EffectContextHandle); 
			ShowDamageText(Props, LocalIncomingDamage, bIsCriticalHit);
		}
	}
}

void USoulAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, Health, OldHealth);	
}

void USoulAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, Stamina, OldStamina);	
}

void USoulAttributeSet:: OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, MaxHealth, OldMaxHealth);	
}

void USoulAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, MaxStamina, OldMaxStamina);	
}

void USoulAttributeSet::OnRep_HealthRegeneration(const FGameplayAttributeData& OldHealthRegeneration) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, HealthRegeneration, OldHealthRegeneration);	
}

void USoulAttributeSet::OnRep_StaminaRegeneration(const FGameplayAttributeData& OldStaminaRegeneration) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, StaminaRegeneration, OldStaminaRegeneration);	
}

void USoulAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, AttackPower, OldAttackPower);	
}

void USoulAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldDefense) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, Defense, OldDefense);	
}

void USoulAttributeSet::OnRep_CriticalRate(const FGameplayAttributeData& OldCriticalRate) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, CriticalRate, OldCriticalRate);	
}

void USoulAttributeSet::OnRep_CriticalDamage(const FGameplayAttributeData& OldCriticalDamage) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, CriticalDamage, OldCriticalDamage);	
}

void USoulAttributeSet::OnRep_CooldownReduction(const FGameplayAttributeData& OldCooldownReduction) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, CooldownReduction, OldCooldownReduction);	
}

void USoulAttributeSet::OnRep_DamageAmplification(const FGameplayAttributeData& OldDamageAmplification) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, DamageAmplification, OldDamageAmplification);	
}

void USoulAttributeSet::OnRep_PhysicalResistance(const FGameplayAttributeData& OldPhysicalResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, PhysicalResistance, OldPhysicalResistance);	
}

void USoulAttributeSet::OnRep_SinResistance(const FGameplayAttributeData& OldSinResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, SinResistance, OldSinResistance);	
}

void USoulAttributeSet::OnRep_VirtueResistance(const FGameplayAttributeData& OldVirtueResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, VirtueResistance, OldVirtueResistance);	
}

void USoulAttributeSet::OnRep_GraceResistance(const FGameplayAttributeData& OldGraceResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, GraceResistance, OldGraceResistance);	
}

void USoulAttributeSet::OnRep_HateResistance(const FGameplayAttributeData& OldHateResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, HateResistance, OldHateResistance);	
}

void USoulAttributeSet::OnRep_TormentResistance(const FGameplayAttributeData& OldTormentResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, TormentResistance, OldTormentResistance);	
}

void USoulAttributeSet::OnRep_SolaceResistance(const FGameplayAttributeData& OldSolaceResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USoulAttributeSet, SolaceResistance, OldSolaceResistance);	
}

void USoulAttributeSet::SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props) const
{
	/* GameplayEffect 발생 시 데이터를 캐싱한 구조체를 OutValue.
	 * EffectContextHandle - Source Data - Target Data 순으로 캐싱. */
	
	Props.EffectContextHandle = Data.EffectSpec.GetContext();
	
	Props.SourceASC = Props.EffectContextHandle.GetOriginalInstigatorAbilitySystemComponent();
	if (IsValid(Props.SourceASC) && Props.SourceASC->AbilityActorInfo.IsValid() && Props.SourceASC->AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.SourceAvatarActor = Props.SourceASC->AbilityActorInfo->AvatarActor.Get();
		Props.SourceController = Props.SourceASC->AbilityActorInfo->PlayerController.Get();
		if (Props.SourceController == nullptr && Props.SourceAvatarActor != nullptr)
		{
			if (const APawn* Pawn = Cast<APawn>(Props.SourceAvatarActor))
			{
				Props.SourceController = Pawn->GetController();
			}
		}
		if (Props.SourceController)
		{
			Props.SourceCharacter = Cast<ACharacter>(Props.SourceController->GetPawn());
		}
	}

	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.TargetAvatarActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		Props.TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		Props.TargetCharacter = Cast<ACharacter>(Props.TargetAvatarActor);
		Props.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Props.TargetAvatarActor);
	}
}

void USoulAttributeSet::ShowDamageText(const FEffectProperties& Props, float DamageAmount, bool bIsCriticalHit) const
{
	/* Damage Types 플로우
	 * Damage Types는 Instigator측의 정보에 해당, GameplayEffectContext 구조체에 정보값을 추가해 Types에서 재정의.
	 * Types에서 Damage Types에 해당하는 플래그를 추가하고, Getter/Setter로 접근.
	 * Custom GameplayEffectContext를 등록하기 위해 AbilitySystemGlobals에서 Alloc을 커스텀 GEC로 교체.
	 * SetTypes/IsTypes 계열은 블루프린트에서 쓸일이 없을거같기는 하지만, 우선 간편한 접근 + 캐스팅을 위해 라이브러리에 정의
	 * IsTypes는 이곳에서, SetTypes는 ExecCal쪽에서 라이브러리 함수에 접근해서 GE Context의 필드 Setup. */
	
	if (Props.SourceCharacter == Props.TargetCharacter) return;
	
	ASoulPlayerController* PC = Cast<ASoulPlayerController>(UGameplayStatics::GetPlayerController(Props.SourceCharacter, 0));
	if (PC) PC->ShowDamageText(DamageAmount, Props.TargetCharacter, bIsCriticalHit);
}