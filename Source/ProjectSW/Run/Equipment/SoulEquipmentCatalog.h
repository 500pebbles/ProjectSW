#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulEquipmentCatalog.generated.h"

class USoulEquipmentDefinition;
class USoulCostumeDefinition;

/** 장비 목록 (DA_EquipmentCatalog). 기본 무기, 전체 무기, 기본 의상, 전체 의상 */
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

	/** 세이브와 선택이 없을 때 입는 의상 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costume")
	TObjectPtr<USoulCostumeDefinition> DefaultCostume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costume")
	TArray<TObjectPtr<USoulCostumeDefinition>> Costumes;

	/** DT 행 이름으로 무기를 찾는다 */
	USoulEquipmentDefinition* FindWeaponByRow(FName RowName) const;

	/** 에셋 이름(DA_Weapon_*)이나 그 뒷부분으로 무기를 찾는다 (치트, 기록 아이콘) */
	USoulEquipmentDefinition* FindWeaponByName(const FString& Name) const;

	/** 에셋 이름(DA_Costume_*)이나 그 뒷부분으로 의상을 찾는다 */
	USoulCostumeDefinition* FindCostumeByName(const FString& Name) const;
};
