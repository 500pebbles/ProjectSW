// 


#include "SoulAbilitySystemLibrary.h"

#include "AbilitySystemComponent.h"
#include "SoulGameplayEffectTypes.h"
#include "AI/NavigationSystemBase.h"
#include "Engine/OverlapResult.h"
#include "Game/SoulGameModeBase.h"
#include "Interface/SoulCombatInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Player/SoulPlayerState.h"
#include "UI/HUD/SoulHUD.h"
#include "UI/WidgetController/SoulWidgetController.h"

UOverlayWidgetController* USoulAbilitySystemLibrary::GetOverlayWidgetController(const UObject* WorldContextObject)
{
	/* 목적은 현재 HUD의 OverlayWidgetController 주소를 받아오기 위함.
	 * 주소의 사용처 : OverlayWidgetController가 보유하는 GAS데이터를 다른 UI에 띄우기 위해 Controller를 빌려옴. 
	 * 최소 PlayerController가 존재하는 UI단위 블루프린트에서 사용, 
	 * 현재 PlayerController를 타고 올라가 이곳에서 HUD, PS, ASC, ASC를 순차적으로 획득.
	 * 이미 존재하는 OverlayWidgetController의 주소를 받아오거나 획득한 포인터를 기반으로 새로 생성하고 주소 획득. */
		
	APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!IsValid(PC)) return nullptr;
	ASoulHUD* SoulHUD = Cast<ASoulHUD>(PC->GetHUD());
	if (!IsValid(SoulHUD)) return nullptr;
	
	ASoulPlayerState* PS = PC->GetPlayerState<ASoulPlayerState>();
	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	UAttributeSet* AS = PS->GetAttributeSet();
	const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
	
	return SoulHUD->GetOverlayWidgetController(WidgetControllerParams);
}

UAttributeMenuWidgetController* USoulAbilitySystemLibrary::GetAttributeMenuWidgetController(const UObject* WorldContextObject)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!IsValid(PC)) return nullptr;
	ASoulHUD* SoulHUD = Cast<ASoulHUD>(PC->GetHUD());
	if (!IsValid(SoulHUD)) return nullptr;
	
	ASoulPlayerState* PS = PC->GetPlayerState<ASoulPlayerState>();
	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	UAttributeSet* AS = PS->GetAttributeSet();
	const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
	
	return SoulHUD->GetAttributeMenuWidgetController(WidgetControllerParams);
}

void USoulAbilitySystemLibrary::EnemyInitializeDefaultAttributes(const UObject* WorldContextObject, EEnemyType EnemyType, float Level,  UAbilitySystemComponent* ASC)
{
	USoulEnemyTypeInfo* EnemyTypeInfo = GetEnemyTypeInfo(WorldContextObject);
	const FEnemyDefaultInfo EnemyDefaultInfo = EnemyTypeInfo->GetEnemyDefaultInfo(EnemyType);
	
	AActor* AvatarActor = ASC->GetAvatarActor();
	
	FGameplayEffectContextHandle DefaultAttributesContextHandle = ASC->MakeEffectContext();
	DefaultAttributesContextHandle.AddSourceObject(AvatarActor);	
	const FGameplayEffectSpecHandle DefaultAttributeSpecHandle = ASC->MakeOutgoingSpec(EnemyDefaultInfo.DefaultAttributes, Level, DefaultAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*DefaultAttributeSpecHandle.Data.Get());
	
	FGameplayEffectContextHandle InitializeAttributesContextHandle = ASC->MakeEffectContext();
	InitializeAttributesContextHandle.AddSourceObject(AvatarActor);	
	const FGameplayEffectSpecHandle InitializeAttributeSpecHandle = ASC->MakeOutgoingSpec(EnemyTypeInfo->InitializeAttributes, Level, InitializeAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*InitializeAttributeSpecHandle.Data.Get());
	
	FGameplayEffectContextHandle ResistanceAttributesContextHandle = ASC->MakeEffectContext();
	InitializeAttributesContextHandle.AddSourceObject(AvatarActor);	
	const FGameplayEffectSpecHandle ResistanceAttributesSpecHandle = ASC->MakeOutgoingSpec(EnemyTypeInfo->ResistanceAttributes, Level, ResistanceAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*ResistanceAttributesSpecHandle.Data.Get());

}

void USoulAbilitySystemLibrary::EnemyGiveStartupAbilities(const UObject* WorldContextObject, UAbilitySystemComponent* ASC, EEnemyType EnemyType)
{
	// Setup. 
	USoulEnemyTypeInfo* EnemyTypeInfo = GetEnemyTypeInfo(WorldContextObject);	
	const FEnemyDefaultInfo& EnemyDefaultInfo = EnemyTypeInfo->GetEnemyDefaultInfo(EnemyType);
	
	// BasicAbility.
	for (auto AbilityClass : EnemyTypeInfo->BasicAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		ASC->GiveAbility(AbilitySpec);
	}
	
	// StartupAbility.
	for (auto AbilityClass : EnemyDefaultInfo.StartupAbilities)
	{
		ISoulCombatInterface* CombatInterface = Cast<ISoulCombatInterface>(ASC->GetAvatarActor());
		if (CombatInterface)
		{
			FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, CombatInterface->GetCharacterLevel());
			ASC->GiveAbility(AbilitySpec);
		}		
	}
}

USoulEnemyTypeInfo* USoulAbilitySystemLibrary::GetEnemyTypeInfo(const UObject* WorldContextObject)
{
	ASoulGameModeBase* SoulGameMode = Cast<ASoulGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (!IsValid(SoulGameMode)) return nullptr;
	return SoulGameMode->EnemyTypeInfo;
}

bool USoulAbilitySystemLibrary::IsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle)
{
	const FSoulGameplayEffectContext* SoulEffectContext = static_cast<const FSoulGameplayEffectContext*>(EffectContextHandle.Get());
	if (!SoulEffectContext) return false;
	
	return SoulEffectContext->IsCriticalHit();
}

void USoulAbilitySystemLibrary::SetIsCriticalHit(FGameplayEffectContextHandle& EffectContextHandle, bool bIsCriticalHit)
{
	FSoulGameplayEffectContext* SoulEffectContext = static_cast<FSoulGameplayEffectContext*>(EffectContextHandle.Get());
	if (!SoulEffectContext) return;

	SoulEffectContext->SetIsCriticalHit(bIsCriticalHit);
}

void USoulAbilitySystemLibrary::GetActorsWithinRadius(const UObject* WorldContextObject, TArray<AActor*>& OutOverlappingActors, const TArray<AActor*>& ActorsToIgnore, float Radius, const FVector& SphereOrigin)
{
	FCollisionQueryParams SphereParams;
	SphereParams.AddIgnoredActors(ActorsToIgnore);

	TArray<FOverlapResult> OverlapResults;
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		World->OverlapMultiByObjectType(OverlapResults, SphereOrigin, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects), FCollisionShape::MakeSphere(Radius), SphereParams);
		for (FOverlapResult& OverlapResult : OverlapResults)
		{
			if (OverlapResult.GetActor()->Implements<USoulCombatInterface>() && !ISoulCombatInterface::Execute_IsDead(OverlapResult.GetActor()))
			{
				OutOverlappingActors.AddUnique(ISoulCombatInterface::Execute_GetAvatarActor(OverlapResult.GetActor()));
			}
		}
	}
}

bool USoulAbilitySystemLibrary::AreActorsAllies(AActor* FirstActor, AActor* SecondActor)
{
	const bool BothArePlayers = FirstActor->ActorHasTag(FName("Player")) && SecondActor->ActorHasTag(FName("Player"));
	const bool BothAreEnemies = FirstActor->ActorHasTag(FName("Enemy")) && SecondActor->ActorHasTag(FName("Enemy"));
	const bool bResult = BothArePlayers || BothAreEnemies;
	
	return bResult;	
}