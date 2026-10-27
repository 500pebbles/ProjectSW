// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulCharacterTypeInfo.generated.h"

class UGameplayEffect;

UENUM(BlueprintType)
enum class ECharacterType : uint8
{
	CharacterType_01,
	CharacterType_02,
	CharacterType_03,
	CharacterType_04
};

USTRUCT(Blueprintable)
struct FCharacterDefaultInfo
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<UGameplayEffect> DefaultAttributes;
};

UCLASS()
class PROJECTSW_API USoulCharacterTypeInfo : public UDataAsset
{
	GENERATED_BODY()
	
public:
	FORCEINLINE FCharacterDefaultInfo GetCharacterDefaultInfo(ECharacterType CharacterType) const { return CharacterTypeInformation.FindChecked(CharacterType); }
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TMap<ECharacterType, FCharacterDefaultInfo> CharacterTypeInformation;	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<UGameplayEffect> InitializeAttributes;		
};