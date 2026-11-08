#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulEquipmentCatalog.generated.h"

class USoulEquipmentDefinition;

/** 장비 목록 (DA_EquipmentCatalog). 기본 무기와 전체 무기. 의상은 단계 E에서 더한다 */
UCLASS(BlueprintType)
class PROJECTSW_API USoulEquipmentCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 시작 무기 (물리만) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USoulEquipmentDefinition> DefaultWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TArray<TObjectPtr<USoulEquipmentDefinition>> Weapons;

	/** DT 행 이름으로 무기를 찾는다 */
	USoulEquipmentDefinition* FindWeaponByRow(FName RowName) const;

	/** 에셋 이름(DA_Weapon_*)이나 그 뒷부분으로 무기를 찾는다 (치트) */
	USoulEquipmentDefinition* FindWeaponByName(const FString& Name) const;
};
