#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputCoreTypes.h"
#include "SoulPresentationData.generated.h"

class UAnimSequenceBase;
class UNiagaraSystem;
class USoundBase;
class USoulKeyHintWidget;
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Banner")
	FLinearColor ClearBannerColor = FLinearColor(1.f, 0.85f, 0.4f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Banner")
	FLinearColor FailedBannerColor = FLinearColor(0.9f, 0.2f, 0.2f);

	/** 포탈이 열릴 때 배너 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FText PortalOpenText = NSLOCTEXT("SoulRun", "PortalOpen", "포탈이 열렸습니다");

	/** 포탈 배너 아랫줄. {Key}는 상호작용 키 이름 (예: {Key} 다음 층) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FText PortalOpenSubtitleFormat = NSLOCTEXT("SoulRun", "PortalOpenSub", "{Key} 다음 층");

	/** 포탈 위 화면 표시. {Key}는 상호작용 키 이름 (예: 포탈 · {Key}) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FText PortalMarkerFormat = NSLOCTEXT("SoulRun", "PortalMarker", "포탈 · {Key}");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FLinearColor PortalBannerColor = FLinearColor(0.75f, 0.45f, 1.f);

	/** 런 층마다 화면 구석에 띄우는 지도 키 안내. {Key}는 지도 키 이름. 비우면 안 띄운다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FText MapKeyHintFormat = NSLOCTEXT("SoulRun", "MapKeyHint", "{Key} 지도");

	/** 화면 기준 앵커와 정렬 (1,0 = 오른쪽 위) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FVector2D MapKeyHintAnchor = FVector2D(1.f, 0.f);

	/** 앵커에서 떨어진 자리 (px) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FVector2D MapKeyHintOffset = FVector2D(-24.f, 120.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FLinearColor MapKeyHintColor = FLinearColor(0.9f, 0.9f, 0.9f);

	/** 지도 키 안내 위젯 (WBP로 바꿀 때). 비우면 코드 기본 위젯 USoulKeyHintWidget */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TSubclassOf<USoulKeyHintWidget> MapKeyHintClass;

	/** 글자 크기. 0이면 위젯 기본값 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map", meta = (ClampMin = "0"))
	int32 MapKeyHintFontSize = 16;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	int32 MapKeyHintZOrder = 5;

	/** 배경음 (D39). 마을에 들어오면. 비우면 조용히. 지금은 합성 드론 임시 SW_BGM_*_Temp라 파일을 받으면 이 칸만 바꾼다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM")
	TObjectPtr<USoundBase> TownBgm;

	/** 던전 층이 준비되면 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM")
	TObjectPtr<USoundBase> DungeonBgm;

	/** 보스 인트로가 끝나면 (인트로 동안과 보스 처치 뒤에는 멈춤) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM")
	TObjectPtr<USoundBase> BossBgm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM", meta = (ClampMin = "0", ClampMax = "2"))
	float BgmVolume = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM", meta = (ClampMin = "0", Units = "s"))
	float BgmFadeInSec = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM", meta = (ClampMin = "0", Units = "s"))
	float BgmFadeOutSec = 1.f;

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

	/** 등장 클립 블렌드 인, 아웃 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (ClampMin = "0", Units = "s"))
	float BossIntroClipBlendIn = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (ClampMin = "0", Units = "s"))
	float BossIntroClipBlendOut = 0.2f;

	/** 카메라가 움직이기 시작하고 네임카드가 뜨기까지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (ClampMin = "0", Units = "s"))
	float BossIntroNameCardDelaySec = 0.f;

	/** 스킵 때 인트로 소리(경고, 문, 보스 소리)를 줄이는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (ClampMin = "0", Units = "s"))
	float BossIntroSkipSoundFadeSec = 0.3f;
};
