// 


#include "SoulRangedAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Actor/SoulProjectileBase.h"
#include "Interface/SoulCombatInterface.h"
#include "Utility/SoulGameplayTags.h"


void USoulRangedAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	
}

void USoulRangedAbility::SpawnProjectile()
{
	if (!GetAvatarActorFromActorInfo()->HasAuthority()) return;
	
	// Step 1 : Setup Projectile Actor. 
	ISoulCombatInterface* CombatInterface = Cast<ISoulCombatInterface>(GetAvatarActorFromActorInfo());
	if (!CombatInterface) return;
	
	const FVector SocketLocation = CombatInterface->GetCombatSocketLocation();
	const FVector ActorForwardDirection = GetAvatarActorFromActorInfo()->GetActorForwardVector();
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(SocketLocation);
	SpawnTransform.SetRotation(ActorForwardDirection.Rotation().Quaternion());	

	ASoulProjectileBase* Projectile = GetWorld()->SpawnActorDeferred<ASoulProjectileBase>(ProjectileClass, SpawnTransform, GetOwningActorFromActorInfo(), Cast<APawn>(GetOwningActorFromActorInfo()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	// Step 2 : Setup GameplayEffect Instance. 
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetAvatarActorFromActorInfo());
	FGameplayEffectContextHandle EffectContextHandle = SourceASC->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), EffectContextHandle);
	
	const float Damage = AbilityDamage.GetValueAtLevel(GetAbilityLevel());
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, SoulTags::Combats::Damage, Damage);
	
	Projectile->DamageEffectSpecHandle = SpecHandle;  // Actor가 보유할 GE -> GA에서 참조해 생성시 전달. 
	
	// Step 3 : Finish Spawn. 
	Projectile->FinishSpawning(SpawnTransform);	
}