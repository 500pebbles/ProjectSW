#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Item/Effect/ItemUseDefinition.h"
#include "SoulEquipmentDefinition.generated.h"

class UGameplayEffect;
class USkeletalMesh;
class UStaticMesh;
class UTexture2D;

/**
 * 무기 정의 (DA_Weapon_*). 플러그인 UItemUseDefinition 자식이라 DT_RunItems 행의 UseDefinition에 그대로 넣는다.
 * Effects에 USoulItemEffect_EquipWeapon 하나를 두면 장비창 장착과 해제가 USoulEquipmentComponent로 이어진다.
 * 이름과 아이콘은 DT 행에도 있다(플러그인 행 구조체가 export되지 않아 ProjectSW에서 못 읽는다). 고칠 때 둘 다 고친다.
 */
UCLASS(BlueprintType)
class PROJECTSW_API USoulEquipmentDefinition : public UItemUseDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 손에 드는 메시 (Weapon 컴포넌트) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMesh> WeaponMesh;

	/** 칼집 메시 (Saya 컴포넌트) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMesh> SheathMesh;

	/** 속성 피해 태그 (SoulTags.DamageTypes.*). 비우면 물리만 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (Categories = "SoulTags.DamageTypes"))
	FGameplayTag ElementTag;

	/** 물리 피해의 몇 배를 속성 피해로 더 넣는가 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
	float ElementRatio = 0.f;

	/** 장착 중 거는 추가 GE (Infinite). 비워도 된다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UGameplayEffect> ExtraEffect;

	/** 이 무기의 DT_RunItems 행 (드랍, 기록용). 기본 무기는 비운다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FDataTableRowHandle ItemRow;
};
