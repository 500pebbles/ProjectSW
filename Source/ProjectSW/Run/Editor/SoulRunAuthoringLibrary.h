#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SoulRunAuthoringLibrary.generated.h"

class UAnimBlueprint;
class UAnimationAsset;
class UAnimSequence;
class UBlendSpace;
class UAnimMontage;
class UAnimSequenceBase;
class USkeletalMeshComponent;

/**
 * 저작 스크립트(Docs/RunAuthoring)가 Python으로 못 쓰는 값을 쓰는 도우미. 에디터에서만 동작하고 게임에서는 아무것도 하지 않는다.
 */
UCLASS()
class PROJECTSW_API USoulRunAuthoringLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 블렌드 스페이스 축(0~2)의 이름, 범위, 격자 수 */
	UFUNCTION(BlueprintCallable, Category = "Soul|Authoring")
	static bool SetBlendSpaceAxis(UBlendSpace* BlendSpace, int32 AxisIndex, const FString& DisplayName, float Min, float Max, int32 GridNum);

	/** 블렌드 스페이스에 샘플 추가 (1D면 Y는 0). RateScale은 샘플 재생 속도 */
	UFUNCTION(BlueprintCallable, Category = "Soul|Authoring")
	static bool AddBlendSpaceSample(UBlendSpace* BlendSpace, UAnimSequence* Animation, float X, float Y, float RateScale);

	/** 템플릿 ABP 자식의 부모 노드 에셋 덮어쓰기. 노드 Guid는 같은 부모를 쓰는 다른 ABP의 첫 덮어쓰기에서 복사한다 */
	UFUNCTION(BlueprintCallable, Category = "Soul|Authoring")
	static bool CopyParentAssetOverride(UAnimBlueprint* Target, const UAnimBlueprint* GuidSource, UAnimationAsset* NewAsset);

	/** 몽타주 첫 슬롯 첫 세그먼트를 원본 클립의 [StartTime, EndTime] 구간으로 자른다 */
	UFUNCTION(BlueprintCallable, Category = "Soul|Authoring")
	static bool TrimMontage(UAnimMontage* Montage, float StartTime, float EndTime);

	/** 몽타주 첫 슬롯의 세그먼트를 Clips 순서대로 이어 붙인 것으로 바꾼다 (SABreak Start, Loop, End). 길이와 섹션을 다시 맞추고 전체 길이를 돌려준다. 실패하면 -1 */
	UFUNCTION(BlueprintCallable, Category = "Soul|Authoring")
	static float SetMontageSegments(UAnimMontage* Montage, const TArray<UAnimSequenceBase*>& Clips);

	/** 에디터 월드의 스켈레탈 메시에 애니 한 프레임을 실제로 평가해 본을 갱신한다 (캡처가 T자세로 나오지 않게) */
	UFUNCTION(BlueprintCallable, Category = "Soul|Authoring")
	static bool EvaluatePose(USkeletalMeshComponent* Component, UAnimSequenceBase* Animation, float Time);

	/** 클래스 칸(TSubclassOf)을 이름으로 쓴다. 인스턴스 편집이 막힌 칸(EQS 테스트의 Context 등)용 */
	UFUNCTION(BlueprintCallable, Category = "Soul|Authoring")
	static bool SetClassProperty(UObject* Object, FName PropertyName, UClass* Value);

	/** ABP의 부모 노드 에셋 덮어쓰기 수 (확인용) */
	UFUNCTION(BlueprintCallable, Category = "Soul|Authoring")
	static int32 GetParentAssetOverrideCount(const UAnimBlueprint* AnimBlueprint);
};
