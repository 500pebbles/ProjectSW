#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SoulTitlePlayerController.generated.h"

class USoulTitleWidget;
class USoundBase;

/**
 * 타이틀 (BP_TitlePlayerController). 아무 키(키보드, 마우스 버튼과 휠, 패드 버튼과 스틱)를 받으면 마을로 간다.
 * 마우스 이동과 IgnoredKeys는 무시한다. OnAnyKey는 BlueprintNativeEvent.
 */
UCLASS()
class PROJECTSW_API ASoulTitlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Title")
	void OnAnyKey(FKey Key);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Title")
	TArray<FKey> IgnoredKeys;

	/** 패드 스틱은 이만큼 넘어가야 키로 본다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Title", meta = (ClampMin = "0", ClampMax = "1"))
	float StickThreshold = 0.f;

	/** 화면이 뜬 뒤 이 시간 전 입력은 무시 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Title", meta = (ClampMin = "0", Units = "s"))
	float MinInputDelay = 0.f;

	/** 키를 받은 뒤 위젯이 사라지는 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Title", meta = (ClampMin = "0", Units = "s"))
	float OutTime = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Title")
	TSubclassOf<USoulTitleWidget> TitleWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Title")
	TObjectPtr<USoundBase> StartSound;

private:
	UPROPERTY()
	TObjectPtr<USoulTitleWidget> TitleWidget;

	double ShownTime = 0.0;
	bool bStarted = false;
};
