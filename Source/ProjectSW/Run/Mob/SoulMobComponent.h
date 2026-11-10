#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Run/Mob/SoulMobTypes.h"
#include "SoulMobComponent.generated.h"

class ASoulCharacterEnemy;
class UAnimMontage;
class UCapsuleComponent;
class UMaterialInstanceDynamic;
class UStaticMesh;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoulMobEventSignature, ASoulCharacterEnemy*, Mob);

/**
 * 런 몹 하나의 값과 동작 (BP_MobBase의 컴포넌트, BP_Mob_<이름>이 값을 덮어쓴다).
 * 레벨 결정, 공격 간격과 공격 고르기, 사망 몽타주, 등장 몽타주, 소환. 없으면 팀 몹 흐름 그대로다.
 */
UCLASS(ClassGroup = (Soul), meta = (BlueprintSpawnableComponent))
class PROJECTSW_API USoulMobComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USoulMobComponent();

	static USoulMobComponent* Find(const AActor* Actor);

	/** 지금 공격해도 되는지 (공격 간격) */
	bool CanAttackNow() const;

	/** 대상과 거리로 공격 몽타주를 고른다. 패턴 인터페이스가 있으면 그쪽을 부른다 */
	UAnimMontage* ChooseAttack(AActor* Target);

	/** 공격 시작을 기록한다 (몽타주 쿨타임) */
	void NotifyAttackStarted(UAnimMontage* Montage);

	/** 공격 끝을 기록한다. AttackInterval은 여기부터 잰다 (클립 + 간격) */
	void NotifyAttackEnded();

	/** 마지막 공격 시작이나 끝 시각 (정체 구제가 사격 중인 몹을 건너뛰는 데 쓴다) */
	double GetLastAttackTime() const { return LastAttackTime; }

	/** 정체 구제: 이 시간 동안 이동도 공격도 없고 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Rescue", meta = (ClampMin = "0", Units = "s"))
	float RescueStuckSeconds = 0.f;

	/** 정체 구제: 플레이어와 이 거리보다 멀면 플레이어 근처 이 반경 안으로 옮긴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Rescue", meta = (ClampMin = "0", Units = "cm"))
	float RescueDistance = 0.f;

	/** 공격 규칙 배율 x BaseDamageScale x 층 배율 */
	float GetDamageMultiplier(UAnimMontage* Montage) const;

	UFUNCTION(BlueprintPure, Category = "Mob")
	ESoulMobRole GetRole() const { return Role; }

	UFUNCTION(BlueprintPure, Category = "Mob")
	int32 GetScoreValue() const { return ScoreValue; }

	UFUNCTION(BlueprintPure, Category = "Mob")
	float GetExpWeight() const { return ExpWeight; }

	UFUNCTION(BlueprintPure, Category = "Mob")
	bool IsStationary() const { return bStationary; }

	/** 강화 몹(정예)인지. 스폰 때 층 규칙(MobRules.Tough)으로 정해진다 */
	UFUNCTION(BlueprintPure, Category = "Mob")
	bool IsTough() const { return bTough; }

	/** 정예로 키운 크기 배율 (BP 기본 크기 대비, 정예가 아니면 1). 공격 판정 위치와 반경에 곱한다 */
	float GetSizeScale() const { return bTough ? ToughRules.MeshScale : 1.f; }

	/** 소환자가 소환물을 만들 때 레벨을 정한다 (InitializeComponent 전에) */
	void SetPendingLevel(int32 InLevel) { PendingLevel = InLevel; }

	/** 몹 BP 클래스에 붙은 이 컴포넌트의 기본값(자식 BP가 덮은 값 우선). 스폰 전에 값을 읽을 때 쓴다 */
	static const USoulMobComponent* FindDefaults(const UClass* EnemyClass);

	/** 방 액터에서 Center 주변(수평 Radius, 바닥에서 3m 아래)의 Meshes 인스턴스와 메시를 숨기고 효과, 조명, 소리를 끈다. 정리한 수 */
	static int32 ClearRoomProps(AActor* Room, const FVector& Center, float FloorZ, float Radius, const TArray<TObjectPtr<UStaticMesh>>& Meshes);

	float GetSpawnClearRadius() const { return SpawnClearRadius; }
	const TArray<TObjectPtr<UStaticMesh>>& GetSpawnClearMeshes() const { return SpawnClearMeshes; }

	/** 등장이 끝나 AI가 움직이기 시작할 때 */
	UPROPERTY(BlueprintAssignable, Category = "Mob")
	FSoulMobEventSignature OnMobSpawnFinished;

	/** 죽었을 때 (점수, 경험치, 드랍이 여기에 붙는다) */
	UPROPERTY(BlueprintAssignable, Category = "Mob")
	FSoulMobEventSignature OnMobDied;

protected:
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	ESoulMobRole Role = ESoulMobRole::Normal;

	/** 공격이 끝난 뒤 다음 공격까지 최소 간격 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Attack", meta = (ClampMin = "0", Units = "s"))
	float AttackInterval = 0.f;

	/** 이 몹 전체 피해 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Attack", meta = (ClampMin = "0"))
	float BaseDamageScale = 1.f;

	/** 공격 몽타주별 규칙 (몽타주는 BP의 Attack Montages에 있어야 한다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Attack")
	TArray<FSoulMobAttackRule> AttackRules;

	/** 사망 몽타주. 마지막 포즈를 유지하려면 몽타주의 자동 블렌드아웃을 끈다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Montage")
	TObjectPtr<UAnimMontage> DeathMontage;

	/** 등장 몽타주. 재생하는 동안 AI를 멈춘다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Montage")
	TObjectPtr<UAnimMontage> SpawnMontage;

	/** 켜면 팀 래그돌을 그대로 둔다 (기본은 물리를 끄고 사망 몽타주) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Montage")
	bool bRagdollOnDeath = false;

	/** 켜면 경직(Combats.HitReact) 어빌리티를 막는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	bool bBlockHitReact = false;

	/** 켜면 제자리 몹 (NavMesh 보정과 구제 대상에서 빠진다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	bool bStationary = false;

	/** 체력 임계 소환. 소환물은 방 전투 수에 들지 않고, 소환자가 죽으면 함께 사라진다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
	TArray<FSoulMobSummon> Summons;

	/** 0이 아니면 층 규칙 대신 이 레벨 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Level", meta = (ClampMin = "0"))
	int32 LevelOverride = 0;

	/** 처치 경험치 = KillBase(레벨) x ExpWeight */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Reward", meta = (ClampMin = "0"))
	float ExpWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Reward", meta = (ClampMin = "0"))
	int32 ScoreValue = 0;

	/** 피격 캡슐 반지름. 0이면 없음. 몸이 이동 캡슐보다 큰 몹(보스)에 쓴다.
	 *  맞는 판정(플레이어 공격 구와 겹침)만 하고 이동, 길찾기, 카메라, 다른 충돌과는 부딪히지 않는다. soul.Mob.DebugHit 1이면 노랗게 그린다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Hurtbox", meta = (ClampMin = "0", Units = "cm"))
	float HurtboxRadius = 0.f;

	/** 피격 캡슐 반높이 (반지름보다 작으면 반지름) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Hurtbox", meta = (ClampMin = "0", Units = "cm"))
	float HurtboxHalfHeight = 0.f;

	/** 피격 캡슐 위치 (액터 기준, X 앞) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Hurtbox")
	FVector HurtboxOffset = FVector::ZeroVector;

	/** 보스 스폰 포인트 주변 이 반경(수평) 안을 층 생성 때(입장 전) 정리한다. 0이면 안 함. 바닥에서 3m 아래에 있는 것만 대상이다.
	 *  SpawnClearMeshes 메시는 숨기고(충돌도 끔), 효과(파티클, 나이아가라), 조명, 소리는 끈다. 그 밖의 바닥, 벽, 소품은 그대로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Spawn", meta = (ClampMin = "0", Units = "cm"))
	float SpawnClearRadius = 0.f;

	/** SpawnClearRadius 안에서 숨길 방 메시 (예: 3층 보스방 화로 SM_Fire_Pit) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|Spawn")
	TArray<TObjectPtr<UStaticMesh>> SpawnClearMeshes;

private:
	void CreateHurtbox(ASoulCharacterEnemy* Enemy);
	void DrawHurtbox() const;
	ASoulCharacterEnemy* GetEnemy() const;
	void HandleDied(AActor* DeadActor);
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void FinishSpawn();
	void SpawnSummon(const FSoulMobSummon& Summon);
	const FSoulMobAttackRule* FindRule(const UAnimMontage* Montage) const;
	/** 강화 몹 체력. 팀 BeginPlay가 기본 속성을 넣은 다음 틱에 부른다 */
	void ApplyToughStats();
	/** 강화 몹 오버레이를 붙이거나 뗀다. 바뀐 메시 수 */
	int32 ApplyToughOverlay(bool bOn);

	int32 PendingLevel = 0;
	double LastAttackTime = -1000.0;
	TMap<TObjectPtr<UAnimMontage>, double> NextUseTime;
	TArray<bool> SummonDone;
	TArray<TWeakObjectPtr<AActor>> SpawnedSummons;
	FTimerHandle SpawnTimer;
	bool bDead = false;

	bool bTough = false;
	FSoulMobToughRules ToughRules;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ToughMID;

	UPROPERTY(Transient)
	TObjectPtr<UCapsuleComponent> Hurtbox;

	FTimerHandle HurtboxDebugTimer;
};
