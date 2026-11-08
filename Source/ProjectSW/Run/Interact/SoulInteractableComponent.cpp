#include "Run/Interact/SoulInteractableComponent.h"
#include "Utility/SoulLog.h"

USoulInteractableComponent::USoulInteractableComponent()
{
	// 인벤토리 아이템 트레이스(ItemTrace = GameTraceChannel2)에만 걸린다
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	bHiddenInGame = true;
}

bool USoulInteractableComponent::Interact(APawn* Instigator)
{
	if (!bEnabled)
		return false;

	if (bOneShot)
	{
		bEnabled = false;
	}
	SW_LOG("[INTERACT] %s by %s", *GetOwner()->GetName(), Instigator ? *Instigator->GetName() : TEXT("null"));
	OnInteracted.Broadcast(Instigator);
	return true;
}
