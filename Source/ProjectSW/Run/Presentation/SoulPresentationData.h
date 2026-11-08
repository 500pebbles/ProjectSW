#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputCoreTypes.h"
#include "SoulPresentationData.generated.h"

class UAnimSequenceBase;
class UNiagaraSystem;
class USoundBase;
class UTexture2D;
class USoulRunLoadingScreenWidget;
class USoulBannerWidget;
class USoulBossIntroWidget;

/** 로딩 화면 한 칸 (Town, F1, F2, F3). 일러스트, 제목, 색, 팁 */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulLoadingEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	TObjectPtr<UTexture2D> Illustration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	FText Subtitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	FLinearColor Color = FLinearColor::White;

	/** 게이지 위 한 줄. 비우면 숨김 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	FText Tip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading", meta = (ClampMin = "0", Units = "s"))
	float FadeIn = 0.f;

	/** 최소 표시 (게이지는 이 시간으로 찬다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading", meta = (ClampMin = "0", Units = "s"))
	float MinShow = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading", meta = (ClampMin = "0", Units = "s"))
	float FadeOut = 0.f;
};

/** 보스 한 마리의 인트로 (보스 BP 클래스 이름으로 찾는다) */
USTRUCT(BlueprintType)
struct PROJECTSW_API FSoulBossIntro
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	FText Title;

	/** 등장 클립과 구간 (초). 끝이 0이면 클립 끝까지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<UAnimSequenceBase> Clip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (ClampMin = "0", Units = "s"))
	float ClipStart = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (ClampMin = "0", Units = "s"))
	float ClipEnd = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<UNiagaraSystem> FX;

	/** 인트로 카메라: 보스 정면 거리와 높이 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (Units = "cm"))
	float CameraDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (Units = "cm"))
	float CameraHeight = 0.f;
};

/** 연출 값 (DA_Presentation, #1, #3, 4.12). 소리와 FX는 비우면 조용히 넘어간다 */
UCLASS(BlueprintType)
class PROJECTSW_API USoulPresentationData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	const FSoulLoadingEntry* FindLoading(FName Id) const;
	const FSoulBossIntro* FindBossIntro(const UObject* Boss) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	TArray<FSoulLoadingEntry> LoadingEntries;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	TSubclassOf<USoulRunLoadingScreenWidget> LoadingWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Banner")
	TSubclassOf<USoulBannerWidget> BannerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Banner", meta = (ClampMin = "0", Units = "s"))
	float BannerHoldSec = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Banner")
	FText FloorClearText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Banner")
	FText DungeonClearText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Banner")
	FText FailedText;

	/** 사망 뒤 FAILED 배너까지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Banner", meta = (ClampMin = "0", Units = "s"))
	float FailedBannerDelaySec = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Banner")
	FText WarningText;

	/** 게이트 F (입장) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> EntrySound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> EntryVoice;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<UNiagaraSystem> EntryFX;

	/** 포탈 F (층 이동) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> FloorMoveSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> FloorMoveVoice;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<UNiagaraSystem> FloorMoveFX;

	/** 층 도착 착지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> LandSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<UNiagaraSystem> LandFX;

	/** 보스방 입장 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> BossDoorSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> WarningSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> FloorClearSound;

	/** 포탈 등장 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> PortalSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<UNiagaraSystem> PortalFX;

	/** 보스 인트로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	TSubclassOf<USoulBossIntroWidget> BossIntroWidgetClass;

	/** 키: 보스 BP 클래스 이름(_C 뺌) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	TMap<FName, FSoulBossIntro> BossIntros;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	TArray<FKey> BossIntroSkipKeys;

	/** 0이면 스킵 끔 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (ClampMin = "0", Units = "s"))
	float BossIntroSkipHoldSec = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (ClampMin = "0", Units = "s"))
	float BossIntroSkipBlendSec = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	FText BossIntroSkipHint;

	/** 카메라 들어갈 때와 돌아올 때 블렌드 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (ClampMin = "0", Units = "s"))
	float BossIntroBlendSec = 0.f;

	/** 레터박스와 경고가 뜨고 카메라가 움직이기까지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (ClampMin = "0", Units = "s"))
	float BossIntroLeadSec = 0.f;

	/** 클립 뒤 머무는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (ClampMin = "0", Units = "s"))
	float BossIntroHoldSec = 0.f;
};
