#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulCostumeDefinition.generated.h"

class UAnimSequence;
class UGameplayEffect;
class USkeletalMesh;
class UStaticMesh;
class UTexture2D;

/** 의상 정의 (DA_Costume_*, 4.7). 몸 메시, 능력치 GE, 이동 배율, 석상 표시 */
UCLASS(BlueprintType)
class PROJECTSW_API USoulCostumeDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costume")
	FText DisplayName;

	/** 카드 문구: 이로운 수치 / 해로운 수치. GE를 고치면 같이 고친다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costume")
	FText StatSummary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costume")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 입으면 바뀌는 몸 메시 (8종 모두 기본 스켈레톤) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costume")
	TObjectPtr<USkeletalMesh> BodyMesh;

	/** 석상이 드는 세트 무기와 칼집 (표시만, 실제 무기는 던전에서) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costume")
	TObjectPtr<USkeletalMesh> StatueWeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costume")
	TObjectPtr<UStaticMesh> StatueSheathMesh;

	/** 입는 동안 거는 GE (Infinite) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costume")
	TArray<TSubclassOf<UGameplayEffect>> EquipEffects;

	/** 걷기와 달리기 속도 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costume", meta = (ClampMin = "0"))
	float MoveSpeedMultiplier = 1.f;

	/** 무기 속성 몫 배율 (#11 CostumeScale) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costume", meta = (ClampMin = "0"))
	float ElementDamageMultiplier = 1.f;

	/** 석상 포즈 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Statue")
	TObjectPtr<UAnimSequence> StatuePose;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Statue", meta = (ClampMin = "0", Units = "s"))
	float StatuePoseTime = 0.f;
};
