// 


#include "SoulRangedAbility.h"

#include "Actor/SoulProjectileBase.h"
#include "Interface/SoulCombatInterface.h"


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

	// Step 2 : Setup Gameplay Effect. 
	
	
	// Step 3 : Finish Spawn. 
	Projectile->FinishSpawning(SpawnTransform);
}
