// 

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemComponent.generated.h"


class UItemData;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UItemComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	FString GetItemName();
	
	FString GetItemDescription();
	
	void GetItemEffect();
	
	FORCEINLINE UItemData* GetItemData() const { return ItemData; }
	
	
private:
	UPROPERTY(EditDefaultsOnly, Category="Item")
	TObjectPtr<UItemData> ItemData;
};
