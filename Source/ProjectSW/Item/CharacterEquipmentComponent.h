// 

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Player/EquipmentComponent.h"
#include "CharacterEquipmentComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSW_API UCharacterEquipmentComponent : public UEquipmentComponent
{
	GENERATED_BODY()

protected:
	virtual int32 GetOwnerLevel() const override;
};
