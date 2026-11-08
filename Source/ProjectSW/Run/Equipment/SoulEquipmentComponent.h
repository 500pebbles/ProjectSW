#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayEffectTypes.h"
#include "SoulEquipmentComponent.generated.h"

class USoulEquipmentCatalog;
class USoulEquipmentDefinition;
class UInventoryComponent;
class USkeletalMesh;
class UStaticMesh;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoulWeaponChangedSignature, USoulEquipmentDefinition*, Weapon);

/**
 * 런 폰의 눈에 보이는 장비와 전투 수치 (BP_RunPlayerCharacter에만 붙는다).
 * 슬롯과 아이템 수명은 플러그인 UEquipmentComponent가 맡고, 이 컴포넌트는 메시 교체와 속성 피해만 맡는다.
 * 팀 폰에는 없으므로 ApplyMeleeDamage의 FindComponentByClass가 null이라 기존 동작 그대로다.
 */
UCLASS(ClassGroup = (Soul), meta = (BlueprintSpawnableComponent))
class PROJECTSW_API USoulEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USoulEquipmentComponent();

	static USoulEquipmentComponent* Find(const AActor* Actor);

	/** 무기 메시와 칼집을 바꾸고 추가 GE를 건다 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void EquipWeapon(USoulEquipmentDefinition* Weapon);

	/** 지금 무기가 Weapon이면 기본 무기로 돌린다 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void UnequipWeapon(USoulEquipmentDefinition* Weapon);

	/** 지금 무기. 기본 무기면 Catalog.DefaultWeapon (없으면 null) */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	USoulEquipmentDefinition* GetCurrentWeapon() const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	USoulEquipmentCatalog* GetCatalog() const { return Catalog; }

	/** 근접 피해 Spec에 무기 속성 몫을 더한다: 물리 x ElementRatio x ElementDamageScale */
	void AppendWeaponDamage(const FGameplayEffectSpecHandle& SpecHandle) const;

	/** 의상이 주는 속성 몫 배율 (단계 E의 의상이 정한다) */
	void SetElementDamageScale(float Scale) { ElementDamageScale = Scale; }

	/** 무기 슬롯이 비어 있으면 인벤토리의 무기를 장착한다 (줍기 직후 자동 장착) */
	bool TryAutoEquipFromInventory(UInventoryComponent* Inventory) const;

	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FSoulWeaponChangedSignature OnWeaponChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USoulEquipmentCatalog> Catalog;

	/** 손 무기 스켈레탈 메시 컴포넌트 이름 (팀 캐릭터 C++의 Weapon) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FName WeaponComponentName = TEXT("Weapon");

	/** 칼집 스태틱 메시 컴포넌트 이름 (BP_WorkerPlayerCharacter의 Saya) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FName SheathComponentName = TEXT("Saya");

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Equipment")
	float ElementDamageScale = 1.f;

private:
	void ApplyVisuals(USkeletalMesh* WeaponMesh, UStaticMesh* SheathMesh);
	void ApplyExtraEffect(const USoulEquipmentDefinition* Weapon);

	UPROPERTY()
	TObjectPtr<USoulEquipmentDefinition> EquippedWeapon;

	UPROPERTY()
	TObjectPtr<USkeletalMesh> DefaultWeaponMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> DefaultSheathMesh;

	FActiveGameplayEffectHandle ExtraEffectHandle;
};
