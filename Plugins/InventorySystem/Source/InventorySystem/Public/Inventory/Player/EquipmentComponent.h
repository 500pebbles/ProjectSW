// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "EquipmentComponent.generated.h"

class UItemData;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChangeEquipmentDelegate, FGameplayTag, SlotTag, UItemData*, Item);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UEquipmentComponent();
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnChangeEquipmentDelegate OnChangeEquipment;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
public:
	bool Equip(UItemData* Item, UItemData*& LastEquippedItem);
	
	UItemData* Unequip(const FGameplayTag& EquipmentTag);
	
	bool CanEquip(const UItemData* Item);
	
	UItemData* GetEquippedItem(const FGameplayTag& EquipmentTag);
	
private:
	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UItemData>> EquippedItems;
	
};
