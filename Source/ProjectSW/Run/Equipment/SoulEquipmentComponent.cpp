#include "Run/Equipment/SoulEquipmentComponent.h"
#include "Run/Equipment/SoulEquipmentCatalog.h"
#include "Run/Equipment/SoulEquipmentDefinition.h"
#include "Run/Equipment/SoulEquipmentTags.h"
#include "Run/Equipment/SoulCostumeDefinition.h"
#include "Run/Actor/SoulTownActors.h"
#include "Run/Core/SoulRunSubsystem.h"
#include "Run/Save/SoulHallOfFameSubsystem.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "Player/SoulPlayerController.h"
#include "GameFramework/Character.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
#include "Utility/SoulGameplayTags.h"
#include "Utility/SoulLog.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Inventory/Player/EquipmentComponent.h"
#include "Inventory/Player/InventoryComponent.h"
#include "Kismet/GameplayStatics.h"

namespace SoulEquipment
{
	template <typename T>
	T* FindNamedComponent(const AActor* Owner, FName Name)
	{
		TArray<T*> Components;
		if (Owner)
		{
			Owner->GetComponents(Components);
		}
		for (T* Component : Components)
		{
			if (Component->GetFName() == Name || Component->GetName().StartsWith(Name.ToString()))
				return Component;
		}
		return nullptr;
	}
}

USoulEquipmentComponent::USoulEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

USoulEquipmentComponent* USoulEquipmentComponent::Find(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<USoulEquipmentComponent>() : nullptr;
}

void USoulEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	// 기본 무기 메시는 폰 BP에 있는 그대로를 기억한다 (해제 때 돌아갈 자리)
	if (const USkeletalMeshComponent* Weapon = SoulEquipment::FindNamedComponent<USkeletalMeshComponent>(GetOwner(), WeaponComponentName))
	{
		DefaultWeaponMesh = Weapon->GetSkeletalMeshAsset();
	}
	if (const UStaticMeshComponent* Sheath = SoulEquipment::FindNamedComponent<UStaticMeshComponent>(GetOwner(), SheathComponentName))
	{
		DefaultSheathMesh = Sheath->GetStaticMesh();
	}
	if (Catalog && Catalog->DefaultWeapon)
	{
		if (Catalog->DefaultWeapon->WeaponMesh)
		{
			DefaultWeaponMesh = Catalog->DefaultWeapon->WeaponMesh;
		}
		if (Catalog->DefaultWeapon->SheathMesh)
		{
			DefaultSheathMesh = Catalog->DefaultWeapon->SheathMesh;
		}
	}
	ApplyVisuals(DefaultWeaponMesh, DefaultSheathMesh);
}

USoulEquipmentDefinition* USoulEquipmentComponent::GetCurrentWeapon() const
{
	if (EquippedWeapon)
		return EquippedWeapon;
	return Catalog ? Catalog->DefaultWeapon : nullptr;
}

void USoulEquipmentComponent::EquipWeapon(USoulEquipmentDefinition* Weapon)
{
	if (!Weapon)
		return;

	EquippedWeapon = Weapon;
	ApplyVisuals(Weapon->WeaponMesh ? Weapon->WeaponMesh.Get() : DefaultWeaponMesh.Get(), Weapon->SheathMesh ? Weapon->SheathMesh.Get() : DefaultSheathMesh.Get());
	ApplyExtraEffect(Weapon);
	SW_LOG("[WEAPON] equip %s element=%s ratio=%.2f", *Weapon->GetName(), *Weapon->ElementTag.ToString(), Weapon->ElementRatio);
	OnWeaponChanged.Broadcast(Weapon);
}

void USoulEquipmentComponent::UnequipWeapon(USoulEquipmentDefinition* Weapon)
{
	if (!EquippedWeapon || (Weapon && Weapon != EquippedWeapon))
		return;

	SW_LOG("[WEAPON] unequip %s", *EquippedWeapon->GetName());
	EquippedWeapon = nullptr;
	ApplyVisuals(DefaultWeaponMesh, DefaultSheathMesh);
	ApplyExtraEffect(Catalog ? Catalog->DefaultWeapon.Get() : nullptr);
	OnWeaponChanged.Broadcast(GetCurrentWeapon());
}

void USoulEquipmentComponent::ApplyVisuals(USkeletalMesh* WeaponMesh, UStaticMesh* SheathMesh)
{
	if (USkeletalMeshComponent* Weapon = SoulEquipment::FindNamedComponent<USkeletalMeshComponent>(GetOwner(), WeaponComponentName); Weapon && WeaponMesh)
	{
		Weapon->SetSkeletalMeshAsset(WeaponMesh);
	}
	if (UStaticMeshComponent* Sheath = SoulEquipment::FindNamedComponent<UStaticMeshComponent>(GetOwner(), SheathComponentName); Sheath && SheathMesh)
	{
		Sheath->SetStaticMesh(SheathMesh);
	}
}

void USoulEquipmentComponent::ApplyExtraEffect(const USoulEquipmentDefinition* Weapon)
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
		return;
	if (ExtraEffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ExtraEffectHandle);
		ExtraEffectHandle.Invalidate();
	}
	if (Weapon && Weapon->ExtraEffect)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(GetOwner());
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Weapon->ExtraEffect, 1.f, Context);
		if (Spec.IsValid())
		{
			ExtraEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

void USoulEquipmentComponent::AppendWeaponDamage(const FGameplayEffectSpecHandle& SpecHandle) const
{
	const USoulEquipmentDefinition* Weapon = GetCurrentWeapon();
	if (!Weapon || !Weapon->ElementTag.IsValid() || Weapon->ElementRatio <= 0.f || !SpecHandle.IsValid())
		return;

	// Physical에는 이미 공격력 비율과 스킬 배율이 곱해져 있어 레벨과 의상 공격력이 속성 몫에도 따라간다
	const float Physical = SpecHandle.Data->GetSetByCallerMagnitude(SoulTags::DamageTypes::Physical, false, 0.f);
	if (Physical <= 0.f)
		return;
	const float Existing = SpecHandle.Data->GetSetByCallerMagnitude(Weapon->ElementTag, false, 0.f);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, Weapon->ElementTag, Existing + Physical * Weapon->ElementRatio * ElementDamageScale);
}

FName USoulEquipmentComponent::GetCurrentCostumeId() const
{
	return EquippedCostume ? EquippedCostume->GetFName() : NAME_None;
}

void USoulEquipmentComponent::ApplySelectedCostume()
{
	const USoulRunSubsystem* Run = USoulRunSubsystem::Get(this);
	USoulCostumeDefinition* Costume = Catalog && Run ? Catalog->FindCostumeByName(Run->GetSelectedCostume().ToString()) : nullptr;
	if (!Costume && Catalog)
	{
		Costume = Catalog->DefaultCostume;
	}
	if (Costume)
	{
		EquipCostume(Costume, false);
	}
}

void USoulEquipmentComponent::EquipCostume(USoulCostumeDefinition* Costume, bool bRemember)
{
	if (!Costume)
		return;
	APawn* Pawn = Cast<APawn>(GetOwner());
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!ASC)
	{
		SW_LOG_WARNING("[COSTUME] ASC 없음, %s 못 입음", *Costume->GetName());
		return;
	}

	// 1. 옛 GE 제거, 새 GE
	for (const FActiveGameplayEffectHandle& Handle : CostumeEffectHandles)
	{
		ASC->RemoveActiveGameplayEffect(Handle);
	}
	CostumeEffectHandles.Reset();
	for (const TSubclassOf<UGameplayEffect>& Effect : Costume->EquipEffects)
	{
		if (!Effect)
			continue;
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(Pawn);
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Effect, 1.f, Context);
		if (Spec.IsValid())
		{
			CostumeEffectHandles.Add(ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()));
		}
	}
	EquippedCostume = Costume;
	ElementDamageScale = Costume->ElementDamageMultiplier;

	// 이동 배율 (속성이 없어 컨트롤러가 걷기와 달리기에 곱한다)
	if (ASoulPlayerController* PC = Pawn ? Cast<ASoulPlayerController>(Pawn->GetController()) : nullptr)
	{
		PC->SetMoveSpeedMultiplier(Costume->MoveSpeedMultiplier);
	}

	// 2. 체력 채움 (MaxHealth가 바뀌어도 Health는 자동으로 안 바뀐다)
	if (RefillEffect)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(Pawn);
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(RefillEffect, 1.f, Context);
		if (Spec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	else
	{
		ASC->SetNumericAttributeBase(USoulAttributeSet::GetHealthAttribute(), ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute()));
	}

	// 3. 잠시 뒤 몸 메시 (8종 모두 기본 스켈레톤이라 AnimInstance, 소켓, 몽타주가 유지된다)
	TWeakObjectPtr<USoulCostumeDefinition> WeakCostume = Costume;
	auto SwapMesh = [this, WeakCostume]()
	{
		ACharacter* Character = Cast<ACharacter>(GetOwner());
		if (Character && WeakCostume.IsValid() && WeakCostume->BodyMesh)
		{
			Character->GetMesh()->SetSkeletalMeshAsset(WeakCostume->BodyMesh);
		}
	};
	if (bRemember && MeshSwapDelay > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(MeshSwapTimer, FTimerDelegate::CreateWeakLambda(this, SwapMesh), MeshSwapDelay, false);
	}
	else
	{
		SwapMesh();
	}

	// 4. 기록, 저장, 석상 틴트
	if (USoulRunSubsystem* Run = USoulRunSubsystem::Get(this))
	{
		Run->SetSelectedCostume(Costume->GetFName());
	}
	if (bRemember)
	{
		if (USoulHallOfFameSubsystem* HoF = USoulHallOfFameSubsystem::Get(this))
		{
			HoF->SetLastCostume(Costume->GetFName());
		}
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ChangeFX, GetOwner()->GetActorLocation());
		UGameplayStatics::PlaySoundAtLocation(this, ChangeSound, GetOwner()->GetActorLocation());
	}
	ASoulCostumeStatue::RefreshAll(GetWorld());
	SW_LOG("[COSTUME] equip %s effects=%d move=%.2f hp=%.0f/%.0f atk=%.1f def=%.1f", *Costume->GetName(), CostumeEffectHandles.Num(), Costume->MoveSpeedMultiplier,
		ASC->GetNumericAttribute(USoulAttributeSet::GetHealthAttribute()), ASC->GetNumericAttribute(USoulAttributeSet::GetMaxHealthAttribute()),
		ASC->GetNumericAttribute(USoulAttributeSet::GetAttackPowerAttribute()), ASC->GetNumericAttribute(USoulAttributeSet::GetDefenseAttribute()));
	OnCostumeChanged.Broadcast(Costume);
}

bool USoulEquipmentComponent::TryAutoEquipFromInventory(UInventoryComponent* Inventory, FName ItemRow) const
{
	if (!Catalog || !Catalog->FindWeaponByRow(ItemRow))
		return false;
	const UEquipmentComponent* Slots = Inventory ? Inventory->GetEquippedItemComponent() : nullptr;
	if (!Slots || !Catalog || const_cast<UEquipmentComponent*>(Slots)->GetEquippedItem(SoulEquipmentTags::Slot_Weapon))
		return false;

	// 플러그인 FInventoryDataArray와 UItemData가 export되지 않아 반사로 항목을 읽는다
	const FStructProperty* DataProp = FindFProperty<FStructProperty>(UInventoryComponent::StaticClass(), TEXT("InventoryData"));
	const FArrayProperty* ArrayProp = DataProp ? FindFProperty<FArrayProperty>(DataProp->Struct, TEXT("ItemDataArray")) : nullptr;
	const FStructProperty* EntryProp = ArrayProp ? CastField<FStructProperty>(ArrayProp->Inner) : nullptr;
	const FStructProperty* IdProp = EntryProp ? FindFProperty<FStructProperty>(EntryProp->Struct, TEXT("ID")) : nullptr;
	const FObjectProperty* ItemProp = EntryProp ? FindFProperty<FObjectProperty>(EntryProp->Struct, TEXT("Item")) : nullptr;
	if (!IdProp || !ItemProp)
		return false;

	FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(DataProp->ContainerPtrToValuePtr<void>(Inventory)));
	for (int32 Index = Helper.Num() - 1; Index >= 0; --Index)
	{
		const uint8* Entry = Helper.GetRawPtr(Index);
		const UObject* Item = ItemProp->GetObjectPropertyValue_InContainer(Entry);
		const FStructProperty* RowProp = Item ? FindFProperty<FStructProperty>(Item->GetClass(), TEXT("ItemRowHandle")) : nullptr;
		const FDataTableRowHandle* Row = RowProp ? RowProp->ContainerPtrToValuePtr<FDataTableRowHandle>(Item) : nullptr;
		if (Row && Row->RowName == ItemRow)
		{
			const FGuid Id = *IdProp->ContainerPtrToValuePtr<FGuid>(Entry);
			SW_LOG("[WEAPON] auto equip row=%s", *Row->RowName.ToString());
			return Inventory->EquipItem(Id);
		}
	}
	return false;
}

#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorldAndArgs GSoulCostumeEquip(
	TEXT("soul.Costume.Equip"),
	TEXT("soul.Costume.Equip <DA 이름 끝부분> : 석상과 같은 경로로 의상을 입는다"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		USoulEquipmentComponent* Equipment = USoulEquipmentComponent::Find(UGameplayStatics::GetPlayerPawn(World, 0));
		USoulCostumeDefinition* Costume = Equipment && Equipment->GetCatalog() && Args.Num() > 0 ? Equipment->GetCatalog()->FindCostumeByName(Args[0]) : nullptr;
		if (!Costume)
		{
			SW_LOG_WARNING("soul.Costume.Equip : 의상을 찾지 못함");
			return;
		}
		Equipment->EquipCostume(Costume);
	}));

static FAutoConsoleCommandWithWorldAndArgs GSoulWeaponEquip(
	TEXT("soul.Weapon.Equip"),
	TEXT("soul.Weapon.Equip <DA 이름 끝부분 또는 DT 행> : 인벤토리를 거치지 않고 무기 메시와 속성을 바꾼다. 인자가 없으면 기본 무기"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		USoulEquipmentComponent* Equipment = USoulEquipmentComponent::Find(UGameplayStatics::GetPlayerPawn(World, 0));
		if (!Equipment || !Equipment->GetCatalog())
		{
			SW_LOG_WARNING("soul.Weapon.Equip : 장비 컴포넌트나 카탈로그 없음");
			return;
		}
		if (Args.IsEmpty())
		{
			Equipment->UnequipWeapon(nullptr);
			return;
		}
		if (USoulEquipmentDefinition* Weapon = Equipment->GetCatalog()->FindWeaponByName(Args[0]))
		{
			Equipment->EquipWeapon(Weapon);
		}
		else
		{
			SW_LOG_WARNING("soul.Weapon.Equip : %s 없음", *Args[0]);
		}
	}));
#endif
