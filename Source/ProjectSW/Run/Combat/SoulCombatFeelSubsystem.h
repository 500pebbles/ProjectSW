#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayEffectTypes.h"
#include "SoulCombatFeelSubsystem.generated.h"

class USoulCombatFeelData;
struct FSoulHitFeelRow;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSoulHitConfirmedSignature, AActor*, Source, AActor*, Target, float, Damage, FName, RowName);

/**
 * 타격감 한 곳 (#4). 피해 확정 지점(SoulAttributeSet)에서 불린다. 역경직(액터 시간 배율, 실시간 복귀), 셰이크, 임팩트 FX와 소리,
 * 넉백, 포이즈(경직 차단과 해제), 플레이어 피격 무적, 보스 처치 슬로모. DA_RunConfig.CombatFeel이 없으면(팀 맵) 아무것도 안 한다.
 */
UCLASS()
class PROJECTSW_API USoulCombatFeelSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static USoulCombatFeelSubsystem* Get(const UObject* WorldContextObject);

	/** 피해가 들어간 직후, 경직 발동 전 */
	static void NotifyDamage(AActor* Source, AActor* Target, float Damage, bool bFatal, bool bCritical, const FGameplayEffectContextHandle& Context);

	UPROPERTY(BlueprintAssignable, Category = "Feel")
	FSoulHitConfirmedSignature OnHitConfirmed;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return !FrozenUntil.IsEmpty() || !InvincibleUntil.IsEmpty() || SlomoUntil > 0.0 || !Poise.IsEmpty(); }

private:
	void HandleDamage(AActor* Source, AActor* Target, float Damage, bool bFatal, bool bCritical, const FGameplayEffectContextHandle& Context);
	const USoulCombatFeelData* GetData() const;
	const FSoulHitFeelRow* FindRow(AActor* Source, AActor* Target, float Damage, const FGameplayEffectContextHandle& Context, FName& OutRowName) const;
	void Freeze(AActor* Actor, float Seconds);
	void ApplyPoise(AActor* Target, const FSoulHitFeelRow& Row);
	void ApplyKnockback(AActor* Source, AActor* Target, float Speed, float Time);
	void StartSlomo(float Dilation, float Duration);

	struct FPoiseState
	{
		float Value = 0.f;
		double LastHit = 0.0;
		double CooldownUntil = 0.0;
		bool bBlocked = false;
		bool bReblockNextTick = false;
	};

	TMap<TWeakObjectPtr<AActor>, double> FrozenUntil;
	TMap<TWeakObjectPtr<AActor>, double> InvincibleUntil;
	TMap<TWeakObjectPtr<AActor>, FPoiseState> Poise;

	/** 행 없음 경고를 이미 남긴 행 */
	mutable TSet<FName> WarnedRows;
	double LastAttackerStop = 0.0;
	double LastShake = 0.0;
	double SlomoUntil = 0.0;

	UPROPERTY(Transient)
	mutable TObjectPtr<const USoulCombatFeelData> CachedData;
	mutable bool bDataResolved = false;
};
