#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulEquipmentCatalog.generated.h"

class USoulEquipmentDefinition;
class USoulCostumeDefinition;
class UTexture2D;

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

	/** 기록의 의상과 무기 ID로 아이콘 (결과창과 보드가 같이 쓴다). 없으면 null */
	UTexture2D* FindCostumeIcon(FName CostumeId) const;
	UTexture2D* FindWeaponIcon(FName WeaponId) const;

	/** 이미지 칸에 아이콘을 넣고, 없으면 칸을 접는다 */
	static void SetIcon(class UImage* Image, UTexture2D* Icon);

	/** DA_RunConfig의 카탈로그 */
	static const USoulEquipmentCatalog* GetRunCatalog(const UObject* WorldContextObject);
};
