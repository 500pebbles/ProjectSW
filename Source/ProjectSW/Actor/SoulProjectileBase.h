// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "SoulProjectileBase.generated.h"

class UNiagaraSystem;
class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class PROJECTSW_API ASoulProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	ASoulProjectileBase();
	
protected:
	virtual void BeginPlay() override;
	
	virtual void Destroyed() override;
	
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
/* GAS Section */
public:
	UPROPERTY(BlueprintReadWrite)
	FGameplayEffectSpecHandle DamageEffectSpecHandle;	
	
/* Component Section */	
public:	
	UPROPERTY()
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Sphere;
	
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<UNiagaraSystem> HitEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TObjectPtr<USoundBase> HitSound;	

protected:
	bool bAlreadyHitOnLocalMachine = false;
	
	float LifeSpan = 10.f;
};