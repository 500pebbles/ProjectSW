// 

#pragma once

#include "CoreMinimal.h"
#include "SoulWidgetController.h"
#include "AttributeMenuWidgetController.generated.h"

struct FGameplayAttribute;
struct FGameplayTag;
class UAttributeInfo;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAttributeInfoSignature, const FSoulAttributeInfo&, Info);

UCLASS(BlueprintType, Blueprintable)
class PROJECTSW_API UAttributeMenuWidgetController : public USoulWidgetController
{
	GENERATED_BODY()
	
public:
	virtual void BindCallbacksToDependencies() override;
	
	virtual void BroadcastInitialValues() override;
	
	UPROPERTY(BlueprintAssignable)
	FAttributeInfoSignature AttributeInfoDelegate;
	
private:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAttributeInfo> AttributeInfo;
	
	void BroadcastAttributeInfo(const FGameplayTag& Tag, float Value) const;
};