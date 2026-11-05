// 

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemEffect.generated.h"

class UItemData;
/**
 * 아이템 효과를 수행하도록 할 클래스
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class INVENTORYSYSTEM_API UItemEffect : public UObject
{
	GENERATED_BODY()
	
public:
	// 블루프린트에서 구현하고, 블루프린트에서 구현하지 않으면 _Implement 호출됨
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ItemEffect)
	void ActivateItemEffect(AActor* User, UItemData* Item);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ItemEffect)
	void DeactivateItemEffect(AActor* User, UItemData* Item);
};
