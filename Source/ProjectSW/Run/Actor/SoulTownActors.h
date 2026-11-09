#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulTownActors.generated.h"

class APawn;
class UMaterialInstanceDynamic;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class USoulCostumeDefinition;
class USoulInteractableComponent;
class UWidgetComponent;
class UAnimSequence;
class USoulCostumeCardWidget;
class USoulHallOfFamePanelWidget;
class USoulNamePlateWidget;
class USoulTownHintWidget;

/** 던전 게이트 (BP_DungeonGate, L_Town G). F → BeginRun → 던전 */
UCLASS()
class PROJECTSW_API ASoulDungeonGate : public AActor
{
	GENERATED_BODY()

public:
	ASoulDungeonGate();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate")
	TObjectPtr<UStaticMeshComponent> GateMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate")
	TObjectPtr<USoulInteractableComponent> Interactable;

private:
	UFUNCTION()
	void HandleInteracted(APawn* User);
};

/**
 * 의상 석상 (BP_CostumeStatue, L_Town S1~S8). F로 그 의상을 입는다.
 * 포즈는 데이터(StatuePose, 시각)이고 OnConstruction에서 적용해 뷰포트에 바로 보인다.
 * 석상처럼: 원본 재질 + 회색 틴트, 입은 의상만 원색 (TintParameterName을 SetColorParameterValueOnMaterials로)
 */
UCLASS()
class PROJECTSW_API ASoulCostumeStatue : public AActor
{
	GENERATED_BODY()

public:
	ASoulCostumeStatue();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** 지금 입은 의상과 같으면 원색, 아니면 회색 */
	void RefreshTint(bool bInstant);

	/** 월드의 모든 석상 틴트 갱신 (의상을 바꾼 뒤) */
	static void RefreshAll(UWorld* World);

	USoulCostumeDefinition* GetCostume() const { return Costume; }

	/** 의상 카드를 켜고 끈다 (포커스 때, 셀프 테스트) */
	void ShowCard(bool bShow);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statue")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statue")
	TObjectPtr<UStaticMeshComponent> Stand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statue")
	TObjectPtr<USkeletalMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statue")
	TObjectPtr<USkeletalMeshComponent> Weapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statue")
	TObjectPtr<UStaticMeshComponent> Saya;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statue")
	TObjectPtr<USoulInteractableComponent> Interactable;

	/** 의상 카드 (WBP_CostumeCard). 플레이어 트레이스가 이 석상을 잡았을 때만 보인다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statue")
	TObjectPtr<UWidgetComponent> Card;

	/** 비우면 카드가 없다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Statue")
	TSubclassOf<USoulCostumeCardWidget> CardClass;

	/** 포커스를 보는 간격 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Statue", meta = (ClampMin = "0.05", Units = "s"))
	float CardCheckInterval = 0.2f;

	/** 이 석상이 입히는 의상 (L_Town 인스턴스마다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Statue")
	TObjectPtr<USoulCostumeDefinition> Costume;

	/** {Name} 입기 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Statue")
	FText PromptFormat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Statue")
	FName TintParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Statue")
	FLinearColor UnselectedTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Statue")
	FLinearColor SelectedTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Statue", meta = (ClampMin = "0", Units = "s"))
	float TintBlendTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Statue")
	FName WeaponSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Statue")
	FName SheathSocketName;

	/** 파생(1위 석상): 의상 포즈 대신 쓸 포즈 */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> PoseOverride;

	float PoseOverrideTime = 0.f;

	/** 파생(1위 석상): 입은 의상과 상관없이 늘 원색 */
	bool bAlwaysSelected = false;

	void ApplyTint(const FLinearColor& Color);
	void ApplyCostumeVisuals();

private:
	UFUNCTION()
	void HandleInteracted(APawn* User);

	void UpdateCardFocus();

	FTimerHandle CardTimer;
	bool bCardShown = false;

	FLinearColor CurrentTint = FLinearColor::White;
	FLinearColor FromTint = FLinearColor::White;
	FLinearColor TargetTint = FLinearColor::White;
	float BlendElapsed = 0.f;
};

/** 1위 석상 (BP_ChampionStatue, L_Town C). 1위 기록의 의상과 무기, 자랑 포즈, 이름표. 기록이 없으면 숨는다 */
UCLASS()
class PROJECTSW_API ASoulChampionStatue : public ASoulCostumeStatue
{
	GENERATED_BODY()

public:
	ASoulChampionStatue();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Champion")
	TObjectPtr<UWidgetComponent> NamePlate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Champion")
	TObjectPtr<UAnimSequence> ChampionPose;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Champion")
	float ChampionPoseTime = 0.f;

	/** {Name} {Score} {Grade} */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Champion")
	FText NameFormat;

private:
	UFUNCTION()
	void RefreshChampion();
};

/** 명예의 전당 보드 (BP_HallOfFame, L_Town B). 간판 메시 + 월드 위젯(상위 행) + F로 여는 패널 */
UCLASS()
class PROJECTSW_API ASoulHallOfFameBoard : public AActor
{
	GENERATED_BODY()

public:
	ASoulHallOfFameBoard();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Board")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Board")
	TObjectPtr<UStaticMeshComponent> Sign;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Board")
	TObjectPtr<UWidgetComponent> Board;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Board")
	TObjectPtr<USoulInteractableComponent> Interactable;

	/** F로 여는 전체 패널 (WBP_HallOfFame) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Board")
	TSubclassOf<USoulHallOfFamePanelWidget> PanelClass;

private:
	UFUNCTION()
	void Refresh();

	UFUNCTION()
	void HandleInteracted(APawn* User);
};

/** 마을 도착 (BP_TownDirector, L_Town). 상황을 판정하고 OnTownArrived를 부른다 */
UENUM(BlueprintType)
enum class ESoulTownArrival : uint8
{
	FirstLaunch,
	Relaunch,
	ClearReturn,
	FailReturn
};

UCLASS()
class PROJECTSW_API ASoulTownDirector : public AActor
{
	GENERATED_BODY()

public:
	ASoulTownDirector();

	UFUNCTION(BlueprintNativeEvent, Category = "Town")
	void OnTownArrived(ESoulTownArrival Arrival);

	/** 처음 실행 때 띄우는 안내 (WBP_TownHint). 비우면 없다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Town")
	TSubclassOf<USoulTownHintWidget> TownHintClass;

protected:
	virtual void BeginPlay() override;

private:
	void RunTownSelfTest(ESoulTownArrival Arrival);
	void RunTownShots();
};
