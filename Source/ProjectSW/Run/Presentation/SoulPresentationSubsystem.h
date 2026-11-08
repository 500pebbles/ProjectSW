#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SoulPresentationSubsystem.generated.h"

class ACameraActor;
class ASoulCharacterEnemy;
class UNiagaraSystem;
class USoundBase;
class USoulBossIntroWidget;
class USoulPresentationData;

UENUM(BlueprintType)
enum class ESoulBossIntroStage : uint8
{
	Start,
	Camera,
	Clip,
	Return,
	Skipped,
	Finished
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoulBossIntroStageSignature, ESoulBossIntroStage, Stage);

/**
 * 월드 연출 (#1): 배너, 2D와 위치 소리, 보스 인트로(레터박스, WARNING, 카메라, 등장 클립, 이름, 길게 눌러 스킵) + 상단 보스 체력.
 * 값은 DA_RunConfig.Presentation. 비어 있으면 아무것도 안 한다.
 */
UCLASS()
class PROJECTSW_API USoulPresentationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static USoulPresentationSubsystem* Get(const UObject* WorldContextObject);

	const USoulPresentationData* GetData() const;

	void ShowBanner(const FText& Title, const FText& Subtitle, const FLinearColor& Color) const;
	void PlaySound2D(USoundBase* Sound) const;
	void PlayAt(USoundBase* Sound, UNiagaraSystem* FX, const FVector& Location) const;

	/** 보스 인트로를 시작한다 (Exit 방 전투 시작 다음 틱) */
	void PlayBossIntro(ASoulCharacterEnemy* Boss);

	bool IsIntroPlaying() const { return bIntroPlaying; }

	/** 디버그와 자체 확인: 지금 인트로를 스킵한다 */
	void SkipIntro();

	UPROPERTY(BlueprintAssignable, Category = "Presentation")
	FSoulBossIntroStageSignature OnIntroStage;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return bIntroPlaying; }

private:
	void IntroCameraStage();
	void IntroClip();
	void IntroReturn();
	void FinishIntro();
	void SetIntroStage(ESoulBossIntroStage Stage);
	void HandleBossDied(AActor* Boss);

	UPROPERTY(Transient)
	TObjectPtr<USoulBossIntroWidget> BossWidget;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> IntroCameraActor;

	TWeakObjectPtr<ASoulCharacterEnemy> IntroBoss;
	FName IntroKey;
	bool bIntroPlaying = false;
	bool bReturning = false;
	bool bSkipped = false;
	FTimerHandle IntroTimer;
	FTimerHandle ClipStopTimer;
};
