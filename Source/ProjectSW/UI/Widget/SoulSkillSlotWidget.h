#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "GameplayTagContainer.h"
#include "UI/Widget/SoulUserWidget.h"
#include "SoulSkillSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USoulAbilitySystemComponent;

/**
 * 스킬 체인 슬롯 드래그 앤 드롭 시 전달되는 페이로드
 */
UCLASS()
class PROJECTSW_API USoulSkillDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "SkillChain")
	FGameplayTag SourceSlotTag;

	UPROPERTY(BlueprintReadWrite, Category = "SkillChain")
	int32 SourceStepIndex = 0;
};

/**
 * 스킬 슬롯 위젯 (하단 HUD용 & 3x3 스킬 체인 편집창 공용)
 * - FixedStepIndex == -1 : 인게임 HUD 모드 (1-0 -> 1-1 -> 1-2 자동 전환 표시)
 * - FixedStepIndex >= 0  : 스킬 체인 편집창 모드 (고정 단계 표시 + 드래그 앤 드롭 / 클릭 교체 지원)
 */
UCLASS()
class PROJECTSW_API USoulSkillSlotWidget : public USoulUserWidget
{
	GENERATED_BODY()

public:
	// 런타임에 슬롯 태그와 단계 인덱스를 설정할 때 사용
	UFUNCTION(BlueprintCallable, Category = "SkillChain")
	void SetupSlot(FGameplayTag InSlotTag, int32 InFixedStepIndex = -1, bool bInEnableChainEdit = false);

	// 즉시 UI 상태 강제 갱신
	UFUNCTION(BlueprintCallable, Category = "SkillChain")
	void RefreshSlotUI();

	// 클릭 교체(Click-to-Swap) 선택 해제
	UFUNCTION(BlueprintCallable, Category = "SkillChain")
	static void ClearPendingSwapSelection();

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UFUNCTION()
	void HandleSkillChainStateChanged();

	USoulAbilitySystemComponent* GetSoulASC();
	int32 GetSlotNumberFromTag() const;

public:
	// 담당할 슬롯 입력 태그 (예: InputTags.KEY_1, InputTags.KEY_2, InputTags.KEY_3)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillChain")
	FGameplayTag SlotInputTag;

	// -1이면 HUD 모드(현재 체인 단계 자동 추적), 0~2이면 편집창 모드(해당 단계 고정 표시)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillChain")
	int32 FixedStepIndex = -1;

	// true이면 마우스 드래그 앤 드롭 및 클릭으로 스킬 체인 순서 교체 가능
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillChain")
	bool bEnableChainEdit = false;

	// 시계방향 쿨타임 머티리얼 (지정 시 Image_CooldownSweep에 Dynamic Material 자동 생성)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillChain|Material")
	TObjectPtr<UMaterialInterface> CooldownMaterialBase;

	// 머티리얼 내 쿨타임 진행도 파라미터 이름 (기본값: "Percent", 1.0 = 쿨타임 시작, 0.0 = 완료)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillChain|Material")
	FName CooldownPercentParamName = TEXT("Percent");

protected:
	// 스킬 아이콘 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_SkillIcon;

	// 회색 반투명 시계방향 쿨타임 오버레이 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_CooldownSweep;

	// 남은 쿨타임 숫자 텍스트 (예: "4.2", "12")
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Cooldown;

	// 체인 단계 배지 텍스트 (예: "1-0", "1-1", "1-2")
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_StepBadge;

	// 3초 체인 유예 타이머 게이지 바 (선택 사항)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar_ChainTimer;

	// 클릭 교체(Click-to-Swap) 시 선택된 슬롯 강조 테두리 이미지 (선택 사항)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_SelectHighlight;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CooldownMID;

	TWeakObjectPtr<USoulAbilitySystemComponent> CachedSoulASC;

	// 클릭->클릭 교체 방식을 위한 정적 선택 슬롯 포인터
	static TWeakObjectPtr<USoulSkillSlotWidget> PendingSwapSourceWidget;
};