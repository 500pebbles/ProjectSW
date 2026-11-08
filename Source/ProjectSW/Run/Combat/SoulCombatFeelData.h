#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GAS/Data/SoulEnemyTypeInfo.h"
#include "SoulCombatFeelData.generated.h"

class UCameraShakeBase;
class UNiagaraSystem;
class USoundBase;

/** 공격 한 종류의 타격감 (DT_HitFeel 행, 4.11). 행 이름 = GA 클래스 이름(_C 뺌), 평타는 .몽타주 섹션 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulHitFeelRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 공격자 정지 (실시간 초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitFeel", meta = (ClampMin = "0", Units = "s"))
	float AttackerStop = 0.f;

	/** 피격자 정지 (실시간 초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitFeel", meta = (ClampMin = "0", Units = "s"))
	float TargetStop = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitFeel")
	TSubclassOf<UCameraShakeBase> Shake;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitFeel", meta = (ClampMin = "0"))
	float ShakeScale = 1.f;

	/** 포이즈 피해 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitFeel", meta = (ClampMin = "0"))
	float Poise = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitFeel", meta = (ClampMin = "0", Units = "cm/s"))
	float KnockbackSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitFeel", meta = (ClampMin = "0", Units = "s"))
	float KnockbackTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitFeel")
	TObjectPtr<UNiagaraSystem> ImpactFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitFeel")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitFeel", meta = (ClampMin = "0"))
	float Volume = 1.f;
};

/** 몹 종류별 포이즈 (4.11). MaxPoise 0이면 매번 경직, bNeverStagger면 경직 없음 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulPoiseProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poise", meta = (ClampMin = "0"))
	float MaxPoise = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poise", meta = (ClampMin = "0"))
	float RegenPerSec = 0.f;

	/** 마지막 타격 뒤 이 시간이 지나야 회복 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poise", meta = (ClampMin = "0", Units = "s"))
	float RegenDelay = 0.f;

	/** 경직 뒤 다시 경직될 수 있기까지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poise", meta = (ClampMin = "0", Units = "s"))
	float StaggerCooldown = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poise")
	bool bNeverStagger = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poise", meta = (ClampMin = "0"))
	float HitStopScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poise", meta = (ClampMin = "0"))
	float KnockbackScale = 1.f;
};

/** 슬로모 한 종류 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulSlomo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slomo", meta = (ClampMin = "0.01", ClampMax = "1"))
	float Dilation = 1.f;

	/** 실시간 초. 0이면 안 함 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slomo", meta = (ClampMin = "0", Units = "s"))
	float Duration = 0.f;
};

/** 전역 타격감 값 (DA_CombatFeel, 4.11). DA_RunConfig.CombatFeel이 비면 서브시스템은 아무것도 안 한다 */
UCLASS(BlueprintType)
class PROJECTSW_API USoulCombatFeelData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** FSoulHitFeelRow 표 (DT_HitFeel) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feel")
	TObjectPtr<UDataTable> HitFeelTable;

	/** 정지 중 액터 시간 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feel", meta = (ClampMin = "0.001", ClampMax = "1"))
	float FreezeDilation = 1.f;

	/** 광역으로 여러 마리를 맞혀도 공격자는 이 간격에 한 번만 멈춘다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feel", meta = (ClampMin = "0", Units = "s"))
	float AttackerStopCooldown = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feel", meta = (ClampMin = "0", Units = "s"))
	float ShakeCooldown = 0.f;

	/** 치명타일 때 정지와 셰이크 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feel", meta = (ClampMin = "0"))
	float CritScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feel")
	FSoulSlomo BossKillSlomo;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feel")
	FSoulSlomo FinalBossKillSlomo;

	/** 플레이어가 맞은 뒤 무적 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player", meta = (ClampMin = "0", Units = "s"))
	float PlayerHurtInvincibleSec = 0.f;

	/** 최대 체력의 이 비율 이상 맞으면 PlayerHurtHeavy 행 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player", meta = (ClampMin = "0", ClampMax = "1"))
	float PlayerHurtHeavyRatio = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poise")
	TMap<EEnemyType, FSoulPoiseProfile> PoiseProfiles;
};
