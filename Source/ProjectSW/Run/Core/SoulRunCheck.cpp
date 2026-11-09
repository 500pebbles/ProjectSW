#include "Run/Core/SoulRunCheck.h"

#if !UE_BUILD_SHIPPING
#include "Run/Core/SoulRunConfig.h"
#include "Run/Core/SoulRunGameComponent.h"
#include "Run/Combat/SoulCombatFeelData.h"
#include "Run/Equipment/SoulCostumeDefinition.h"
#include "Run/Equipment/SoulEquipmentCatalog.h"
#include "Run/Equipment/SoulEquipmentComponent.h"
#include "Run/Equipment/SoulEquipmentDefinition.h"
#include "Run/Progression/SoulProgressionComponent.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Player/SoulPlayerState.h"
#include "Utility/SoulLog.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameplayEffect.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#endif

bool SoulRunCheck::Log(const UObject* WorldContextObject, const TCHAR* Where)
{
#if UE_BUILD_SHIPPING
	return true;
#else
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
		return false;

	const bool bRun = USoulRunGameComponent::IsRunWorld(World);
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
	const ACharacter* Character = Cast<ACharacter>(Pawn);
	const USoulEquipmentComponent* Equipment = USoulEquipmentComponent::Find(Pawn);
	const USoulProgressionComponent* Progression = USoulProgressionComponent::Find(Pawn);
	const USoulEquipmentCatalog* Catalog = Equipment ? Equipment->GetCatalog() : nullptr;
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	const USpringArmComponent* Arm = Pawn ? Pawn->FindComponentByClass<USpringArmComponent>() : nullptr;
	const UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr;
	const ASoulPlayerState* PS = Pawn ? Pawn->GetPlayerState<ASoulPlayerState>() : nullptr;
	const USkeletalMesh* Body = Character && Character->GetMesh() ? Character->GetMesh()->GetSkeletalMeshAsset() : nullptr;
	const USoulEquipmentDefinition* Weapon = Equipment ? Equipment->GetCurrentWeapon() : nullptr;
	const USoulCostumeDefinition* Costume = Equipment ? Equipment->GetCurrentCostume() : nullptr;

	auto Attr = [ASC](const FGameplayAttribute& Attribute) { return ASC ? ASC->GetNumericAttribute(Attribute) : -1.f; };
	auto NameOf = [](const UObject* Object) { return Object ? Object->GetName() : FString(TEXT("none")); };
	const FString Refill = Catalog && Catalog->RefillEffect ? Catalog->RefillEffect->GetName() : FString(TEXT("none"));
	const int32 Lag = Arm ? (Arm->bEnableCameraLag ? 1 : 0) : -1;

	SW_LOG("[RUNCHECK] where=%s map=%s run=%d pawn=%s eq=%d prog=%d catalog=%s refill=%s costume=%s weapon=%s body=%s crit=%.0f hp=%.0f/%.0f ap=%.0f stamina=%.0f/%.0f walk=%.0f lag=%d/%.0f/%.0f arm=%.0f level=%d",
		Where, *UGameplayStatics::GetCurrentLevelName(World, true), bRun ? 1 : 0, Pawn ? *Pawn->GetClass()->GetName() : TEXT("none"),
		Equipment ? 1 : 0, Progression ? 1 : 0, *NameOf(Catalog), *Refill, *NameOf(Costume), *NameOf(Weapon), *NameOf(Body),
		Attr(USoulAttributeSet::GetCriticalRateAttribute()), Attr(USoulAttributeSet::GetHealthAttribute()), Attr(USoulAttributeSet::GetMaxHealthAttribute()),
		Attr(USoulAttributeSet::GetAttackPowerAttribute()), Attr(USoulAttributeSet::GetStaminaAttribute()), Attr(USoulAttributeSet::GetMaxStaminaAttribute()),
		Move ? Move->MaxWalkSpeed : -1.f, Lag, Arm ? Arm->CameraLagSpeed : -1.f, Arm ? Arm->CameraLagMaxDistance : -1.f, Arm ? Arm->TargetArmLength : -1.f,
		PS ? PS->GetPlayerLevel() : -1);

	if (!bRun)
		return true;

	// 런 월드에서는 팀 폰의 런 컴포넌트와 카탈로그가 있어야 하고, 카메라 랙이 DA_CombatFeel 값이어야 한다
	TArray<FString> Fails;
	if (!Pawn)
		Fails.Add(TEXT("pawn"));
	if (!Equipment)
		Fails.Add(TEXT("eq"));
	if (!Progression)
		Fails.Add(TEXT("prog"));
	if (!Catalog)
		Fails.Add(TEXT("catalog"));
	const USoulRunConfig* Config = USoulRunGameComponent::GetRunConfig(World);
	const USoulCombatFeelData* Feel = Config ? Config->CombatFeel.Get() : nullptr;
	// ApplyRunCamera와 같은 규칙: DA가 랙을 켤 때만 검사한다. DA가 끄면 팀 SpringArm 값을 그대로 두므로 값만 찍는다
	if (Feel && Feel->bCameraLag && Arm)
	{
		if (Lag != 1)
		{
			Fails.Add(TEXT("lag"));
		}
		else if (!FMath::IsNearlyEqual(Arm->CameraLagSpeed, Feel->CameraLagSpeed) || !FMath::IsNearlyEqual(Arm->CameraLagMaxDistance, Feel->CameraLagMaxDistance))
		{
			Fails.Add(TEXT("lag values"));
		}
	}
	if (Fails.Num() > 0)
	{
		SW_LOG_ERROR("[RUNCHECK] FAIL where=%s: %s", Where, *FString::Join(Fails, TEXT(", ")));
		return false;
	}
	return true;
#endif
}

#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorldAndArgs GSoulRunCheck(
	TEXT("soul.Run.Check"),
	TEXT("soul.Run.Check [이름] : 런 월드 여부와 폰의 런 컴포넌트, 치명, 카메라 랙을 [RUNCHECK] 한 줄로"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		SoulRunCheck::Log(World, Args.Num() > 0 ? *Args[0] : TEXT("console"));
	}));
#endif
