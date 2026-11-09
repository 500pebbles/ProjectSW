#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoulProgressionComponent.generated.h"

class AController;
class APawn;
class ASoulCharacterEnemy;
class UAbilitySystemComponent;
class USoulLevelBarWidget;
class USoulMobComponent;
class USoulProgressionData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSoulProgressionChangedSignature, int32, Level, int32, Exp, int32, ExpToNext);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoulLevelUpSignature, int32, NewLevel);

/**
 * 런의 레벨과 경험치. 팀 폰 BP_WorkerPlayerCharacter에 값 없이 붙어 있고 런 월드(USoulRunGameComponent::IsRunWorld)에서만 동작한다.
 * 값은 USoulRunSubsystem의 RunState에 있고 층을 넘어 유지된다.
 * 빙의 다음 틱에 GE_Level_Stats(레벨 스탯과 런 치명 20)를 RunState.Level로 건다. 몹 처치 경험치는 Encounter의 처치 신호로 받는다.
 * BeginPlay에서 DA_CombatFeel의 카메라 랙을 SpringArm에 넣는다.
 */
UCLASS(ClassGroup = (Soul), meta = (BlueprintSpawnableComponent))
class PROJECTSW_API USoulProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USoulProgressionComponent();

	static USoulProgressionComponent* Find(const AActor* Actor);

	/** 경험치를 더하고 넘치면 레벨을 올린다 */
	UFUNCTION(BlueprintCallable, Category = "Level")
	void AddExp(int32 Amount, FName Reason);

	/** 레벨을 바로 정한다 (치트, 확인용) */
	UFUNCTION(BlueprintCallable, Category = "Level")
	void SetLevel(int32 NewLevel);

	UFUNCTION(BlueprintPure, Category = "Level")
	int32 GetLevel() const;

	UFUNCTION(BlueprintPure, Category = "Level")
	int32 GetExp() const;

	UFUNCTION(BlueprintPure, Category = "Level")
	int32 GetExpToNext() const;

	UPROPERTY(BlueprintAssignable, Category = "Level")
	FSoulProgressionChangedSignature OnProgressionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Level")
	FSoulLevelUpSignature OnLevelUp;

	void DumpToLog() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	void ScheduleInitialize();
	void InitializeProgression();
	void ApplyRunCamera() const;
	void ApplyLevel(int32 Level, bool bLevelUp);
	void HandleMobKilled(ASoulCharacterEnemy* Enemy, const USoulMobComponent* Mob);
	void Broadcast();

	const USoulProgressionData* GetData() const;
	UAbilitySystemComponent* GetASC() const;

	UPROPERTY(Transient)
	TObjectPtr<USoulLevelBarWidget> LevelBar;

	FDelegateHandle MobKilledHandle;
	bool bInitialized = false;
};
