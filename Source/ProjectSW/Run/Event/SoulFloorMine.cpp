#include "Run/Event/SoulFloorMine.h"
#include "Run/Event/SoulRoomEvent_Effect.h"
#include "Character/SoulCharacterPlayer.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Utility/SoulGameplayTags.h"
#include "Utility/SoulLog.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

ASoulFloorMine::ASoulFloorMine()
{
	PrimaryActorTick.bCanEverTick = false;

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetCollisionProfileName(TEXT("Trigger"));
	Sphere->SetCanEverAffectNavigation(false);
	SetRootComponent(Sphere);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Sphere);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);

	WarnLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("WarnLight"));
	WarnLight->SetupAttachment(Sphere);
	WarnLight->SetCastShadows(false);
}

void ASoulFloorMine::BeginPlay()
{
	Super::BeginPlay();
	BaseIntensity = WarnLight->Intensity;
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleOverlap);
}

void ASoulFloorMine::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 팀 방 트리거와 같은 판별. 몹은 밟아도 안 터진다
	if (Cast<ASoulCharacterPlayer>(OtherActor))
	{
		Arm();
	}
}

void ASoulFloorMine::ForceTrigger()
{
	Arm();
}

void ASoulFloorMine::Arm()
{
	if (bArmed)
		return;
	bArmed = true;

	UGameplayStatics::PlaySoundAtLocation(this, WarnSound, GetActorLocation());
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, WarnFX, GetActorLocation());
	K2_OnArmed();
	if (BlinkInterval > 0.f)
	{
		GetWorldTimerManager().SetTimer(BlinkTimer, this, &ThisClass::Blink, BlinkInterval, true);
	}
	GetWorldTimerManager().SetTimer(FuseTimer, this, &ThisClass::Detonate, FMath::Max(FuseTime, 0.01f), false);
	SW_LOG("[MINE] armed %s", *GetName());
}

void ASoulFloorMine::Blink()
{
	bBlinkOn = !bBlinkOn;
	WarnLight->SetIntensity(bBlinkOn ? BlinkIntensity : BaseIntensity);
}

void ASoulFloorMine::Detonate()
{
	GetWorldTimerManager().ClearTimer(BlinkTimer);
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BlastFX, GetActorLocation());
	UGameplayStatics::PlaySoundAtLocation(this, BlastSound, GetActorLocation());

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Player);
	if (Player && ASC && FVector::Dist(Player->GetActorLocation(), GetActorLocation()) <= BlastRadius
		&& !ASC->HasMatchingGameplayTag(SoulTags::States::Invincible) && !ASC->HasMatchingGameplayTag(SoulTags::States::Dead))
	{
		const float Before = ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute());
		const float Damage = ASoulRoomEvent_Effect::ComputeRatioDamage(ASC, DamageRatio, bCanKill);
		ASoulRoomEvent_Effect::ApplyMagnitudeEffect(ASC, DamageEffect, MagnitudeTag, Damage);
		SW_LOG("[MINE] blast hit hp %.0f -> %.0f", Before, ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute()));
	}
	else
	{
		SW_LOG("[MINE] blast miss");
	}

	K2_OnDetonated();
	Destroy();
}
