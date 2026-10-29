#pragma once
#include "GameplayEffectTypes.h"
#include "SoulGameplayEffectTypes.generated.h"

USTRUCT(BlueprintType)
struct FSoulGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()
	
public:
	virtual UScriptStruct* GetScriptStruct() const
	{
		return StaticStruct();
	}
	
	virtual FSoulGameplayEffectContext* Duplicate() const
	{
		FSoulGameplayEffectContext* NewContext = new FSoulGameplayEffectContext();
		*NewContext = *this;
		if (GetHitResult())
		{
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}	
	
	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
	

/* Custom Properties Section */	
public:
	void SetIsCriticalHit(bool bInIsCriticalHit) { bIsCriticalHit = bInIsCriticalHit; }
	bool IsCriticalHit() const { return bIsCriticalHit; }
	
protected: 
	UPROPERTY()
	bool bIsCriticalHit = false;
};



/* Templates */
template<>
struct TStructOpsTypeTraits<FSoulGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FSoulGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};