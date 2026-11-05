// 


#include "SoulProjectileBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GAS/Utility/SoulAbilitySystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Utility/SoulDefineMacro.h"

ASoulProjectileBase::ASoulProjectileBase()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	
	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	SetRootComponent(Sphere);
	
	Sphere->SetCollisionObjectType(ECC_Projectile);	
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	ProjectileMovement->InitialSpeed = 1000.f;
	ProjectileMovement->MaxSpeed = 1000.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
}

void ASoulProjectileBase::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(LifeSpan);
	
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
}

void ASoulProjectileBase::Destroyed()
{	
	/* 모든 머신이 사운드 재생과 이펙트 생성은 처리. 서버는 추가로 액터를 파괴하는 로직을 담당한다.
	 * 다른 클라이언트는 자신의 로컬머신에서 아직 Overlap이 이루어지지 않았음에도
	 * 네트워크 지연이 없는 서버측에서 Destroy()를 먼저 호출해 액터가 사라지는 버그가 발생.
	 * bAlreadyHit 플래그를 두고 OnDestroy()에서 이런 상황을 처리하기 위한 분기를 작성 */
	
	if (!bAlreadyHitOnLocalMachine && !HasAuthority())
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation(), FRotator::ZeroRotator);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HitEffect, GetActorLocation());
	}
	
	Super::Destroyed();
}

void ASoulProjectileBase::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{	
	if (DamageEffectSpecHandle.Data.IsValid() && DamageEffectSpecHandle.Data.Get()->GetContext().GetEffectCauser() == OtherActor) return;
	if (USoulAbilitySystemLibrary::AreActorsAllies(DamageEffectSpecHandle.Data.Get()->GetContext().GetEffectCauser(), OtherActor)) return;
	if (!bAlreadyHitOnLocalMachine)
	{		
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation(), FRotator::ZeroRotator);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HitEffect, GetActorLocation());
	}
	

	if (HasAuthority())
	{	
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
		if (TargetASC)
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageEffectSpecHandle.Data.Get()); 
		}
		Destroy();
	}
	else
	{
		bAlreadyHitOnLocalMachine = true;
	}
}