// 


#include "CharacterEquipmentComponent.h"

#include "Character/SoulCharacterPlayer.h"

int32 UCharacterEquipmentComponent::GetOwnerLevel() const
{
	if (ASoulCharacterPlayer* Player = Cast<ASoulCharacterPlayer>(GetOwner()))
	{
		return Player->GetCharacterLevel();   // → PlayerState::Level 반환
	}
	return 1;
}
