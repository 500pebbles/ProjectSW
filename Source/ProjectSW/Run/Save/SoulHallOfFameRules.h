#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulHallOfFameRules.generated.h"

/** 명예의 전당 규칙 (DA_HallOfFameRules) */
UCLASS(BlueprintType)
class PROJECTSW_API USoulHallOfFameRules : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 보관 최대 행 수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame", meta = (ClampMin = "1"))
	int32 MaxEntries = 1;

	/** 마을 보드에 보이는 행 수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame", meta = (ClampMin = "1"))
	int32 BoardRows = 1;

	/** 이름 최대 글자 수 (커밋 때 자른다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame", meta = (ClampMin = "1"))
	int32 NameMaxLength = 1;

	/** 이름을 한 번도 넣지 않았을 때 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame")
	FString DefaultName;

	/** 실패한 런도 기록에 넣을지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HallOfFame")
	bool bIncludeFailures = false;
};
