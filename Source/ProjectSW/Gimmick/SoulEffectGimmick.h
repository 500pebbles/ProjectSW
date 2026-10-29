// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulEffectGimmick.generated.h"

class UGameplayEffect;
class USphereComponent;

UCLASS()
class PROJECTSW_API ASoulEffectGimmick : public AActor
{
	GENERATED_BODY()

public:
	ASoulEffectGimmick();

protected:
	virtual void BeginPlay() override;
	
protected:
	UFUNCTION(BlueprintCallable)
	void ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> EffectToApply);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;
};
