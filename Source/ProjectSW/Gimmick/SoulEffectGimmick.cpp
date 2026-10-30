// 


#include "SoulEffectGimmick.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

ASoulEffectGimmick::ASoulEffectGimmick()
{
}

void ASoulEffectGimmick::BeginPlay()
{
	Super::BeginPlay();
}

void ASoulEffectGimmick::ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> EffectToApply)
{
	const bool bIsEnemy = TargetActor->ActorHasTag(FName("Enemy"));
	if (bIsEnemy) return;
	
	// Interface를 통한 통상적인 ASC 접근.
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(TargetActor);
	if (!ASCInterface) return;
	
	// BlueprintLibrary를 활용한 ASC 접근 방법을 추가로 체크.
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!IsValid(TargetASC)) return;
	
	FGameplayEffectContextHandle EffectContextHandle = TargetASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);
	FGameplayEffectSpecHandle EffectSpecHandle = TargetASC->MakeOutgoingSpec(EffectToApply, 1.f, EffectContextHandle);
	TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	
	Destroy();
}