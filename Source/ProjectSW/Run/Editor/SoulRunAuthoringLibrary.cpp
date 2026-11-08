#include "Run/Editor/SoulRunAuthoringLibrary.h"

#include "Animation/AnimBlueprint.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "UObject/UnrealType.h"

bool USoulRunAuthoringLibrary::SetBlendSpaceAxis(UBlendSpace* BlendSpace, int32 AxisIndex, const FString& DisplayName, float Min, float Max, int32 GridNum)
{
#if WITH_EDITOR
	if (!BlendSpace || AxisIndex < 0 || AxisIndex > 2)
		return false;

	// BlendParameters는 protected라 리플렉션으로 쓴다
	const FStructProperty* Property = FindFProperty<FStructProperty>(UBlendSpace::StaticClass(), TEXT("BlendParameters"));
	if (!Property)
		return false;

	BlendSpace->Modify();
	FBlendParameter* Parameters = Property->ContainerPtrToValuePtr<FBlendParameter>(BlendSpace, AxisIndex);
	Parameters->DisplayName = DisplayName;
	Parameters->Min = Min;
	Parameters->Max = Max;
	Parameters->GridNum = GridNum;
	BlendSpace->ValidateSampleData();
	BlendSpace->ResampleData();
	BlendSpace->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool USoulRunAuthoringLibrary::AddBlendSpaceSample(UBlendSpace* BlendSpace, UAnimSequence* Animation, float X, float Y, float RateScale)
{
#if WITH_EDITOR
	if (!BlendSpace || !Animation)
		return false;

	BlendSpace->Modify();
	const int32 Index = BlendSpace->AddSample(Animation, FVector(X, Y, 0.f));
	if (Index == INDEX_NONE)
		return false;

	// 샘플 재생 속도 (SampleData는 protected, 읽기 접근자만 있다)
	FBlendSample& Sample = const_cast<FBlendSample&>(BlendSpace->GetBlendSamples()[Index]);
	Sample.RateScale = RateScale;
	BlendSpace->ValidateSampleData();
	BlendSpace->ResampleData();
	BlendSpace->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool USoulRunAuthoringLibrary::CopyParentAssetOverride(UAnimBlueprint* Target, const UAnimBlueprint* GuidSource, UAnimationAsset* NewAsset)
{
#if WITH_EDITORONLY_DATA
	if (!Target || !GuidSource || !NewAsset || GuidSource->ParentAssetOverrides.IsEmpty())
		return false;

	const FGuid NodeGuid = GuidSource->ParentAssetOverrides[0].ParentNodeGuid;
	Target->Modify();
	FAnimParentNodeAssetOverride* Existing = Target->ParentAssetOverrides.FindByPredicate([&NodeGuid](const FAnimParentNodeAssetOverride& Override) { return Override.ParentNodeGuid == NodeGuid; });
	if (Existing)
	{
		Existing->NewAsset = NewAsset;
	}
	else
	{
		Target->ParentAssetOverrides.Add(FAnimParentNodeAssetOverride(NodeGuid, NewAsset));
	}
	Target->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

int32 USoulRunAuthoringLibrary::GetParentAssetOverrideCount(const UAnimBlueprint* AnimBlueprint)
{
#if WITH_EDITORONLY_DATA
	return AnimBlueprint ? AnimBlueprint->ParentAssetOverrides.Num() : 0;
#else
	return 0;
#endif
}

bool USoulRunAuthoringLibrary::TrimMontage(UAnimMontage* Montage, float StartTime, float EndTime)
{
#if WITH_EDITOR
	if (!Montage || Montage->SlotAnimTracks.IsEmpty() || Montage->SlotAnimTracks[0].AnimTrack.AnimSegments.IsEmpty() || EndTime <= StartTime)
		return false;

	Montage->Modify();
	FAnimSegment& Segment = Montage->SlotAnimTracks[0].AnimTrack.AnimSegments[0];
	Segment.AnimStartTime = StartTime;
	Segment.AnimEndTime = EndTime;
	Segment.StartPos = 0.f;
	Montage->SetCompositeLength(Montage->CalculateSequenceLength());
	Montage->UpdateLinkableElements();
	Montage->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}
