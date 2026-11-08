#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
class UAudioComponent;

#include "SoulPresentationSubsystem.generated.h"

class ACameraActor;
class ASoulCharacterEnemy;
class UNiagaraSystem;
class USoundBase;
class USoulBossIntroWidget;
class USoulPresentationData;
class UUserWidget;

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

	/** 플레이어 사망: 인트로를 멈추되 입력은 켜지 않는다 */
	void AbortIntroForDeath();

	/** 층을 지울 때: 보스 HP 바와 보스 참조를 정리한다 */
	void ClearBoss();

	/** 배경음을 바꾼다. 같은 소리가 이미 나오면 그대로 둔다. 비면 멈춘다 */
	void PlayBgm(USoundBase* Sound);

	/** 배경음을 줄여 멈춘다. FadeOutSec가 음수면 DA 값 */
	void StopBgm(float FadeOutSec = -1.f);

	USoundBase* GetCurrentBgm() const { return BgmSound; }

	/**
	 * 팀 오버레이(스킬, HP), 팀 인벤토리 HUD(퀵슬롯, 줍기 문구), 레벨 바, 키 안내를 숨기거나 되돌린다. 이유별로 세고 모든 이유가 풀리면 되돌린다.
	 * 위젯 에셋은 고치지 않고 보이기만 바꾼다 (보스 인트로, 결과창)
	 */
	void SetGameHudHidden(FName Reason, bool bHidden);

	UPROPERTY(BlueprintAssignable, Category = "Presentation")
	FSoulBossIntroStageSignature OnIntroStage;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return bIntroPlaying; }

private:
	void IntroCameraStage();
	void IntroClip();
	void IntroReturn();
	void FinishIntro(bool bRestoreInput = true);
	void PlayIntroSound2D(USoundBase* Sound);
	void DisableInputAfterFrame(TWeakObjectPtr<APlayerController> WeakPC, uint64 LockFrame);
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
	FTimerHandle NameCardTimer;

	/** 스킵 때 줄일 인트로 소리 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> IntroSounds;

	bool bWarnedSkipKeys = false;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> BgmComponent;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> BgmSound;

	TSet<FName> HudHideReasons;
	TArray<TPair<TWeakObjectPtr<UUserWidget>, ESlateVisibility>> HiddenHudWidgets;
};
