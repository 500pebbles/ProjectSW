#include "Run/Mob/SoulMobProjectile.h"
#include "GAS/Utility/SoulAbilitySystemLibrary.h"
#include "Utility/SoulGameplayTags.h"
#include "Utility/SoulLog.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

ASoulMobProjectile::ASoulMobProjectile()
{
	// 벽과 바닥에 막히고 폰은 겹침으로 맞힌다
	Sphere->SetCollisionObjectType(ECC_WorldDynamic);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Sphere->SetGenerateOverlapEvents(true);
}

void ASoulMobProjectile::BeginPlay()
{
	if (Radius > 0.f)
	{
		Sphere->SetSphereRadius(Radius);
	}
	if (Speed > 0.f)
	{
		ProjectileMovement->InitialSpeed = Speed;
		ProjectileMovement->MaxSpeed = Speed;
		ProjectileMovement->Velocity = GetActorForwardVector() * Speed;
		if (MaxDistance > 0.f)
		{
			LifeSpan = MaxDistance / Speed;
		}
	}

	Super::BeginPlay();

	// 부모의 겹침 처리(아군 판정 null 크래시, 무적 무시)를 떼고 자기 처리를 단다
	Sphere->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::OnSphereOverlap);
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleOverlap);
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &ThisClass::HandleStop);
}

void ASoulMobProjectile::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !IsValid(OtherActor) || !DamageEffectSpecHandle.IsValid())
		return;

	AActor* Causer = DamageEffectSpecHandle.Data->GetContext().GetEffectCauser();
	if (OtherActor == Causer || OtherActor == GetOwner() || (IsValid(Causer) && USoulAbilitySystemLibrary::AreActorsAllies(Causer, OtherActor)))
		return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (!TargetASC)
		return;

	// 대시 무적과 죽은 대상은 통과한다
	if (TargetASC->HasMatchingGameplayTag(SoulTags::States::Invincible) || TargetASC->HasMatchingGameplayTag(SoulTags::States::Dead))
		return;

	TargetASC->ApplyGameplayEffectSpecToSelf(*DamageEffectSpecHandle.Data.Get());
	SW_LOG("[MOB] projectile hit %s", *OtherActor->GetName());
	Explode();
}

void ASoulMobProjectile::HandleStop(const FHitResult& ImpactResult)
{
	Explode();
}

void ASoulMobProjectile::Explode()
{
	if (bAlreadyHitOnLocalMachine)
		return;
	bAlreadyHitOnLocalMachine = true;

	UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation(), FRotator::ZeroRotator);
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HitEffect, GetActorLocation());
	Destroy();
}
