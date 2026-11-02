// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemUseDefinition.generated.h"

class UItemEffect;
/**
 * 아이템 사용 시 배열에 저장한 아이템 효과들을 일괄로 호출하여 아이템 사용을 처리할 클래스
 */
UCLASS()
class INVENTORYSYSTEM_API UItemUseDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	bool bIsConsumable = false;
	
	UPROPERTY(EditDefaultsOnly, Instanced)
	TArray<TObjectPtr<UItemEffect>> Effects;
};
