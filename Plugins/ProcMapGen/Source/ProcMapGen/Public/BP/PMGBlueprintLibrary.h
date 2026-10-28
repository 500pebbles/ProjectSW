#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/PMGResult.h"
#include "PMGBlueprintLibrary.generated.h"

class UPMGSettings;

/** ProcMapGen의 Blueprint 노드 모음. 실제 일은 C++ 함수(PMG::GenerateMap 등)가 하고, 여기서는 Blueprint에 노출만 한다 */
UCLASS()
class PROCMAPGEN_API UPMGBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 설정과 시드로 맵 구조를 생성한다 (스폰은 하지 않음). 같은 설정 + 같은 Seed면 항상 같은 결과.
	 * 설정 에셋의 Seed를 쓰려면 Settings에서 Seed를 꺼내 연결한다. Settings가 None이면 실패 결과를 돌려준다.
	 */
	UFUNCTION(BlueprintCallable, Category = "ProcMapGen")
	static FPMGResult GenerateMap(const UPMGSettings* Settings, int32 Seed);
};
